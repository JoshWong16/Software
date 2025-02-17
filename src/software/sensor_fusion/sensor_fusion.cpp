#include "software/sensor_fusion/sensor_fusion.h"

#include "software/logger/logger.h"

SensorFusion::SensorFusion(std::shared_ptr<const SensorFusionConfig> sensor_fusion_config)
    : sensor_fusion_config(sensor_fusion_config),
      field(std::nullopt),
      ball(std::nullopt),
      friendly_team(),
      enemy_team(),
      game_state(),
      referee_stage(std::nullopt),
      ball_filter(),
      friendly_team_filter(),
      enemy_team_filter(),
      team_with_possession(TeamSide::ENEMY),
      friendly_goalie_id(0),
      enemy_goalie_id(0),
      reset_time_vision_packets_detected(0),
      last_t_capture(0)
{
    if (!sensor_fusion_config)
    {
        throw std::invalid_argument("SensorFusion created with null SensorFusionConfig");
    }
}

std::optional<World> SensorFusion::getWorld() const
{
    if (field && ball)
    {
        World new_world(*field, *ball, friendly_team, enemy_team);
        new_world.updateGameState(game_state);
        new_world.setTeamWithPossession(team_with_possession);
        if (referee_stage)
        {
            new_world.updateRefereeStage(*referee_stage);
        }


        return new_world;
    }
    else
    {
        return std::nullopt;
    }
}

void SensorFusion::processSensorProto(const SensorProto &sensor_msg)
{
    if (sensor_msg.has_ssl_vision_msg())
    {
        updateWorld(sensor_msg.ssl_vision_msg());
    }

    if (sensor_msg.has_ssl_referee_msg())
    {
        updateWorld(sensor_msg.ssl_referee_msg());
    }

    updateWorld(sensor_msg.robot_status_msgs());

    friendly_team.assignGoalie(friendly_goalie_id);
    enemy_team.assignGoalie(enemy_goalie_id);

    if (sensor_fusion_config->getOverrideGameControllerFriendlyGoalieId()->value())
    {
        RobotId friendly_goalie_id_override =
            sensor_fusion_config->getFriendlyGoalieId()->value();
        friendly_team.assignGoalie(friendly_goalie_id_override);
    }

    if (sensor_fusion_config->getOverrideGameControllerEnemyGoalieId()->value())
    {
        RobotId enemy_goalie_id_override =
            sensor_fusion_config->getEnemyGoalieId()->value();
        enemy_team.assignGoalie(enemy_goalie_id_override);
    }

    if (sensor_fusion_config->getOverrideRefereeCommand()->value())
    {
        std::string previous_state_string =
            sensor_fusion_config->getPreviousRefereeCommand()->value();
        std::string current_state_string =
            sensor_fusion_config->getCurrentRefereeCommand()->value();
        try
        {
            RefereeCommand previous_state =
                fromStringToRefereeCommand(previous_state_string);
            game_state.updateRefereeCommand(previous_state);
            RefereeCommand current_state =
                fromStringToRefereeCommand(current_state_string);
            game_state.updateRefereeCommand(current_state);
        }
        catch (const std::invalid_argument &e)
        {
            LOG(WARNING) << e.what();
        }
    }
}

void SensorFusion::updateWorld(const SSLProto::SSL_WrapperPacket &packet)
{
    if (packet.has_geometry())
    {
        updateWorld(packet.geometry());
    }

    if (packet.has_detection())
    {
        checkForVisionReset(packet.detection().t_capture());
        updateWorld(packet.detection());
    }
}

void SensorFusion::updateWorld(const SSLProto::SSL_GeometryData &geometry_packet)
{
    field = createField(geometry_packet);
    if (!field)
    {
        LOG(WARNING)
            << "Invalid field packet has been detected, which means field may be unreliable "
            << "and the createFieldFromPacketGeometry may be parsing using the wrong proto format";
    }
}

void SensorFusion::updateWorld(const SSLProto::Referee &packet)
{
    if (sensor_fusion_config->getFriendlyColorYellow()->value())
    {
        game_state.updateRefereeCommand(createRefereeCommand(packet, TeamColour::YELLOW));
        friendly_goalie_id = packet.yellow().goalkeeper();
        enemy_goalie_id    = packet.blue().goalkeeper();
    }
    else
    {
        game_state.updateRefereeCommand(createRefereeCommand(packet, TeamColour::BLUE));
        friendly_goalie_id = packet.blue().goalkeeper();
        enemy_goalie_id    = packet.yellow().goalkeeper();
    }

    if (game_state.isOurBallPlacement())
    {
        auto pt = getBallPlacementPoint(packet);
        if (pt)
        {
            game_state.setBallPlacementPoint(*pt);
        }
        else
        {
            LOG(WARNING)
                << "In BALL_PLACEMENT_US game state, but no ball placement point found"
                << std::endl;
        }
    }

    referee_stage = createRefereeStage(packet);
}

void SensorFusion::updateWorld(
    const google::protobuf::RepeatedPtrField<TbotsProto::RobotStatus> &robot_status_msgs)
{
    for (auto &robot_status_msg : robot_status_msgs)
    {
        int robot_id = robot_status_msg.robot_id();
        std::set<RobotCapability> unavailableCapabilities;

        for (const auto &error_code_msg : robot_status_msg.error_code())
        {
            if (error_code_msg == TbotsProto::ErrorCode::WHEEL_0_MOTOR_HOT ||
                error_code_msg == TbotsProto::ErrorCode::WHEEL_1_MOTOR_HOT ||
                error_code_msg == TbotsProto::ErrorCode::WHEEL_2_MOTOR_HOT ||
                error_code_msg == TbotsProto::ErrorCode::WHEEL_3_MOTOR_HOT)
            {
                unavailableCapabilities.insert(RobotCapability::Move);
            }
            else if (error_code_msg == TbotsProto::ErrorCode::LOW_CAP ||
                     error_code_msg == TbotsProto::ErrorCode::CHARGE_TIMEOUT)
            {
                unavailableCapabilities.insert(RobotCapability::Kick);
                unavailableCapabilities.insert(RobotCapability::Chip);
            }
            else if (error_code_msg == TbotsProto::ErrorCode::DRIBBLER_MOTOR_HOT)
            {
                unavailableCapabilities.insert(RobotCapability::Dribble);
            }
        }
        friendly_team.setUnavailableRobotCapabilities(robot_id, unavailableCapabilities);
    }
}

void SensorFusion::updateWorld(const SSLProto::SSL_DetectionFrame &ssl_detection_frame)
{
    double min_valid_x = sensor_fusion_config->getMinValidX()->value();
    double max_valid_x = sensor_fusion_config->getMaxValidX()->value();
    bool ignore_invalid_camera_data =
        sensor_fusion_config->getIgnoreInvalidCameraData()->value();

    // We invert the field side if we explicitly choose to override the values
    // provided by the game controller. The 'defending_positive_side' parameter dictates
    // the side we are defending if we are overriding the value
    const bool override_game_controller_defending_side =
        sensor_fusion_config->getOverrideGameControllerDefendingSide()->value();
    const bool defending_positive_side =
        sensor_fusion_config->getDefendingPositiveSide()->value();
    const bool should_invert_field =
        override_game_controller_defending_side && defending_positive_side;

    bool friendly_team_is_yellow =
        sensor_fusion_config->getFriendlyColorYellow()->value();

    std::optional<Ball> new_ball;
    auto ball_detections = createBallDetections({ssl_detection_frame}, min_valid_x,
                                                max_valid_x, ignore_invalid_camera_data);
    auto yellow_team =
        createTeamDetection({ssl_detection_frame}, TeamColour::YELLOW, min_valid_x,
                            max_valid_x, ignore_invalid_camera_data);
    auto blue_team =
        createTeamDetection({ssl_detection_frame}, TeamColour::BLUE, min_valid_x,
                            max_valid_x, ignore_invalid_camera_data);

    if (should_invert_field)
    {
        for (auto &detection : ball_detections)
        {
            detection = invert(detection);
        }
        for (auto &detection : yellow_team)
        {
            detection = invert(detection);
        }
        for (auto &detection : blue_team)
        {
            detection = invert(detection);
        }
    }

    if (friendly_team_is_yellow)
    {
        friendly_team = createFriendlyTeam(yellow_team);
        enemy_team    = createEnemyTeam(blue_team);
    }
    else
    {
        friendly_team = createFriendlyTeam(blue_team);
        enemy_team    = createEnemyTeam(yellow_team);
    }

    new_ball = createBall(ball_detections);
    if (new_ball)
    {
        updateBall(*new_ball);
    }

    if (ball)
    {
        bool friendly_team_has_ball = teamHasBall(friendly_team, *ball);
        bool enemy_team_has_ball    = teamHasBall(enemy_team, *ball);

        if (friendly_team_has_ball && !enemy_team_has_ball)
        {
            // take defensive view of exclusive possession for friendly possession
            team_with_possession = TeamSide::FRIENDLY;
        }

        if (enemy_team_has_ball)
        {
            team_with_possession = TeamSide::ENEMY;
        }
    }
}

void SensorFusion::updateBall(Ball new_ball)
{
    ball = new_ball;
    game_state.updateBall(*ball);
}

std::optional<Ball> SensorFusion::createBall(
    const std::vector<BallDetection> &ball_detections)
{
    if (field)
    {
        std::optional<Ball> new_ball =
            ball_filter.estimateBallState(ball_detections, field.value().fieldBoundary());
        return new_ball;
    }
    return std::nullopt;
}

Team SensorFusion::createFriendlyTeam(const std::vector<RobotDetection> &robot_detections)
{
    Team new_friendly_team =
        friendly_team_filter.getFilteredData(friendly_team, robot_detections);
    return new_friendly_team;
}

Team SensorFusion::createEnemyTeam(const std::vector<RobotDetection> &robot_detections)
{
    Team new_enemy_team = enemy_team_filter.getFilteredData(enemy_team, robot_detections);
    return new_enemy_team;
}

RobotDetection SensorFusion::invert(RobotDetection robot_detection) const
{
    robot_detection.position =
        Point(-robot_detection.position.x(), -robot_detection.position.y());
    robot_detection.orientation = robot_detection.orientation + Angle::half();
    return robot_detection;
}

BallDetection SensorFusion::invert(BallDetection ball_detection) const
{
    ball_detection.position =
        Point(-ball_detection.position.x(), -ball_detection.position.y());
    return ball_detection;
}

bool SensorFusion::teamHasBall(const Team &team, const Ball &ball)
{
    for (const auto &robot : team.getAllRobots())
    {
        if (robot.isNearDribbler(ball.position()))
        {
            return true;
        }
    }
    return false;
}

void SensorFusion::checkForVisionReset(double t_capture)
{
    if (t_capture < last_t_capture && t_capture < VISION_PACKET_RESET_TIME_THRESHOLD)
    {
        reset_time_vision_packets_detected++;
    }
    else
    {
        reset_time_vision_packets_detected = 0;
        last_t_capture                     = t_capture;
    }

    if (reset_time_vision_packets_detected > VISION_PACKET_RESET_COUNT_THRESHOLD)
    {
        LOG(WARNING) << "Vision reset detected... Resetting SensorFusion!" << std::endl;
        resetWorldComponents();
        last_t_capture                     = 0;
        reset_time_vision_packets_detected = 0;
    }
}

void SensorFusion::resetWorldComponents()
{
    field                = std::nullopt;
    ball                 = std::nullopt;
    friendly_team        = Team();
    enemy_team           = Team();
    game_state           = GameState();
    referee_stage        = std::nullopt;
    ball_filter          = BallFilter();
    friendly_team_filter = RobotTeamFilter();
    enemy_team_filter    = RobotTeamFilter();
    team_with_possession = TeamSide::ENEMY;
}
