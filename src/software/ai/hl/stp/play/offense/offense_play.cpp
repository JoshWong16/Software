#include "software/ai/hl/stp/play/offense/offense_play.h"

#include "shared/constants.h"
#include "shared/parameter/cpp_dynamic_parameters.h"
#include "software/ai/evaluation/calc_best_shot.h"
#include "software/ai/evaluation/possession.h"
#include "software/logger/logger.h"
#include "software/util/generic_factory/generic_factory.h"

OffensePlay::OffensePlay(std::shared_ptr<const AiConfig> config)
    : Play(config, true),
      shoot_or_pass_play(std::make_shared<ShootOrPassPlay>(ai_config)),
      crease_defense_play(std::make_shared<CreaseDefensePlay>(ai_config))

{
}

void OffensePlay::getNextTactics(TacticCoroutine::push_type &yield, const World &world)
{
    // This function doesn't get called so it does nothing
    while (true)
    {
        yield({{}});
    }
}

void OffensePlay::updateTactics(const PlayUpdate &play_update)
{
    PriorityTacticVector tactics_to_return;
    unsigned int num_shoot_or_pass = play_update.num_tactics - 2;
    unsigned int num_defenders     = 2;
    if (play_update.num_tactics <= 3)
    {
        num_shoot_or_pass = 1;
        num_defenders     = play_update.num_tactics - 1;
    }

    shoot_or_pass_play->updateTactics(
        PlayUpdate(play_update.world, num_shoot_or_pass,
                   [&tactics_to_return](PriorityTacticVector new_tactics) {
                       for (const auto &tactic_vector : new_tactics)
                       {
                           tactics_to_return.push_back(tactic_vector);
                       }
                   }));

    crease_defense_play->updateControlParams(
        play_update.world.ball().position(),
        TbotsProto::MaxAllowedSpeedMode::PHYSICAL_LIMIT);

    crease_defense_play->updateTactics(
        PlayUpdate(play_update.world, num_defenders,
                   [&tactics_to_return](PriorityTacticVector new_tactics) {
                       for (const auto &tactic_vector : new_tactics)
                       {
                           tactics_to_return.push_back(tactic_vector);
                       }
                   }));

    play_update.set_tactics(tactics_to_return);
}

// Register this play in the genericFactory
static TGenericFactory<std::string, Play, OffensePlay, AiConfig> factory;
