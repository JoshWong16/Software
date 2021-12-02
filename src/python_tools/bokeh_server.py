from bokeh.layouts import column, row
from bokeh.plotting import figure
from bokeh.sampledata.sea_surface_temperature import sea_surface_temperature
from bokeh.server.server import Server
from bokeh.themes import Theme
from bokeh.plotting import figure
from bokeh.io import output_notebook, show, push_notebook, curdoc
from bokeh.models import ColumnDataSource, Slider, Button, CustomJS, Select, Dropdown

from python_tools.plotting.plot_ssl_wrapper import SSLWrapperPlotter, MM_PER_M

from proto.messages_robocup_ssl_wrapper_pb2 import SSL_WrapperPacket
from python_tools.proto_log import ProtoLog
from proto.ai_control_config_pb2 import AiControlConfig


def bkapp(doc):
    fig = figure(width=1000, height=600, match_aspect=True)
    fig.background_fill_color = "white"
    ssl_wrapper_plotter = SSLWrapperPlotter(fig)

    # TODO: UNIX SOCKET CODE
    def plot_ssl_wrapper_at_idx(idx):
        ssl_wrapper_plotter.plot_ssl_wrapper(wrapper_proto_log[idx])

    # START AI BUTTON
    start_button = Button(label="Start AI")

    def start_clicked(b):
        AiControlConfig.run_ai = "true"
        print(AiControlConfig.run_ai)

    start_button.on_click(start_clicked)

    # GAMESTATE OVERRIDE
    def gamestate(attr, old, new):
        AiControlConfig.override_ai_play = new
        print(AiControlConfig.override_ai_play)

    gamestateOver = Dropdown(
        label="Gamestate Override",
        button_type="warning",
        menu=[
            (""),
            ("Use GameController"),
            ("HALT"),
            ("STOP"),
            ("NORMAL_START"),
            ("FORCE_START"),
            ("PREPARE_KICKOFF_US"),
            ("PREPARE_KICKOFF_THEM"),
            ("PREPARE_PENALTY_US"),
            ("PREPARE_PENALTY_THEM"),
            ("DIRECT_FREE_US"),
            ("DIRECT_FREE_THEM"),
            ("INDIRECT_FREE_US"),
            ("INDIRECT_FREE_THEM"),
            ("TIMEOUT_US"),
            ("TIMEOUT_THEM"),
            ("GOAL_US"),
            ("GOAL_THEM"),
            ("BALL_PLACEMENT_US"),
            ("BALL_PLACEMENT_THEM"),
        ],
    )

    gamestateOver.on_change("menu", gamestate)

    # PLAY OVERRIDE
    def play(atrr, old, new):
        AiControlConfig.current_ai_play = new
        print(AiControlConfig.current_ai_play)

    playOver = Dropdown(
        menu=[(""), ("Use AI Selection"), ("HaltPlay")],
        label="Play Override:",
    )

    playOver.on_change("menu", play)

    doc.add_root(column(fig, row(start_button, gamestateOver, playOver)))

if __name__ == "__main__":
    # Setting num_procs here means we can't touch the IOLoop before now, we must
    # let Server handle that. If you need to explicitly handle IOLoops then you
    # will need to use the lower level BaseServer class.
    server = Server({"/": bkapp}, num_procs=1)
    server.start()

    print("Opening Bokeh application on http://localhost:5006/")

    server.io_loop.add_callback(server.show, "/")
    server.io_loop.start()
