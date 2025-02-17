#include "software/ai/hl/stp/play/hardware_challenge_plays/scoring_from_contested_possession_play.h"

#include "shared/constants.h"
#include "software/ai/hl/stp/tactic/dribble/dribble_tactic.h"
#include "software/ai/hl/stp/tactic/move/move_tactic.h"
#include "software/util/generic_factory/generic_factory.h"

ScoringFromContestedPossessionPlay::ScoringFromContestedPossessionPlay(
    std::shared_ptr<const AiConfig> config)
    : Play(config, false)
{
}

void ScoringFromContestedPossessionPlay::getNextTactics(TacticCoroutine::push_type &yield,
                                                        const World &world)
{
    std::shared_ptr<DribbleTactic> dribble_tactic = std::make_shared<DribbleTactic>();
    dribble_tactic->updateControlParams(std::nullopt, std::nullopt, true);
    std::shared_ptr<MoveTactic> move_tactic = std::make_shared<MoveTactic>();

    do
    {
        TacticVector result = {};
        if (world.gameState().isPlaying())
        {
            // TODO (#2107): implement contested scoring
            result.emplace_back(dribble_tactic);
        }
        else
        {
            // TODO (#2107): implement face ball opposite attacker 0.3m away
            move_tactic->updateControlParams(Point(0, 0), Angle::zero(), 0.0);
            result.emplace_back(move_tactic);
        }
        yield({result});
    } while (true);
}

// Register this play in the genericFactory
static TGenericFactory<std::string, Play, ScoringFromContestedPossessionPlay, AiConfig>
    factory;
