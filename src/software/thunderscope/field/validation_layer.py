import pyqtgraph as pg
import time
from proto.import_all_protos import *

from pyqtgraph.Qt import QtCore, QtGui

from software.thunderscope.colors import Colors
import software.thunderscope.constants as constants
from software.networking.threaded_unix_listener import ThreadedUnixListener
from software.thunderscope.field.field_layer import FieldLayer


class ValidationLayer(FieldLayer):

    PASSED_VALIDATION_PERSISTANCE_TIMEOUT_S = 1.0

    def __init__(self, buffer_size=10):
        FieldLayer.__init__(self)

        # Validation protobufs are generated by simulated tests
        self.eventually_validation_set_receiver = ThreadedUnixListener(
            constants.UNIX_SOCKET_BASE_PATH + "eventually_validation",
            ValidationProtoSet,
            max_buffer_size=buffer_size,
        )
        self.always_validation_set_receiver = ThreadedUnixListener(
            constants.UNIX_SOCKET_BASE_PATH + "always_validation",
            ValidationProtoSet,
            max_buffer_size=buffer_size,
        )

        self.cached_eventually_validation_set = ValidationProtoSet()
        self.cached_always_validation_set = ValidationProtoSet()

        self.passed_validation_timeout_pairs = []

    def draw_validation(self, painter, validation):
        """Draw Validation

        :param painter: The painter object to draw with
        :param validation: Validation proto

        """
        if validation.status == ValidationStatus.PASSING:
            painter.setPen(pg.mkPen(Colors.VALIDATION_PASSED_COLOR, width=3))

        if validation.status == ValidationStatus.FAILING:
            painter.setPen(pg.mkPen(Colors.VALIDATION_FAILED_COLOR, width=3))

        for circle in validation.geometry.circles:
            painter.drawEllipse(
                self.createCircle(
                    constants.MM_PER_M * circle.origin.x_meters,
                    constants.MM_PER_M * circle.origin.y_meters,
                    constants.MM_PER_M * circle.radius,
                )
            )

        for polygon in validation.geometry.polygons:
            polygon_points = [
                QtCore.QPoint(
                    int(constants.MM_PER_M * point.x_meters),
                    int(constants.MM_PER_M * point.y_meters),
                )
                for point in polygon.points
            ]

            poly = QtGui.QPolygon(polygon_points)
            painter.drawPolygon(poly)

    def paint(self, painter, option, widget):
        """Paint this layer

        :param painter: The painter object to draw with
        :param option: Style information (unused)
        :param widget: The widget that we are painting on

        """

        # Receive messages and load from cache
        eventually_validation_set = (
            self.eventually_validation_set_receiver.get_most_recent_message()
        )
        always_validation_set = (
            self.always_validation_set_receiver.get_most_recent_message()
        )

        if not eventually_validation_set:
            eventually_validation_set = self.cached_eventually_validation_set
        if not always_validation_set:
            always_validation_set = self.cached_always_validation_set

        self.cached_eventually_validation_set = eventually_validation_set
        self.cached_always_validation_set = always_validation_set

        # Draw Always Validation
        for validation in always_validation_set.validations:
            self.draw_validation(painter, validation)

        # Draw Eventually Validation
        for validation in eventually_validation_set.validations:
            if validation.status == ValidationStatus.PASSING:
                if validation.validation_type == ValidationType.EVENTUALLY:
                    self.passed_validation_timeout_pairs.append(
                        (
                            validation,
                            time.time()
                            + ValidationLayer.PASSED_VALIDATION_PERSISTANCE_TIMEOUT_S,
                        )
                    )

            self.draw_validation(painter, validation)

        # Draw cached validation
        for validation, stop_drawing_time in list(self.passed_validation_timeout_pairs):
            if time.time() < stop_drawing_time:
                self.draw_validation(painter, validation)
            else:
                self.passed_validation_timeout_pairs.remove(
                    (validation, stop_drawing_time)
                )
