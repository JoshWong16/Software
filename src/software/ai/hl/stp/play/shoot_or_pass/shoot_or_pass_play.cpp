#include "software/ai/hl/stp/play/shoot_or_pass/shoot_or_pass_play.h"

#include "shared/constants.h"
#include "shared/parameter/cpp_dynamic_parameters.h"
#include "software/logger/logger.h"
#include "software/util/generic_factory/generic_factory.h"

ShootOrPassPlay::ShootOrPassPlay(std::shared_ptr<const AiConfig> config)
    : Play(config, true), fsm{ShootOrPassPlayFSM{config}}, control_params{}
{
}

void ShootOrPassPlay::getNextTactics(TacticCoroutine::push_type &yield,
                                     const World &world)
{
    // This function doesn't get called so it does nothing
    while (true)
    {
        yield({{}});
    }
}

void ShootOrPassPlay::updateTactics(const PlayUpdate &play_update)
{
    fsm.process_event(ShootOrPassPlayFSM::Update(control_params, play_update));
}

// Register this play in the genericFactory
static TGenericFactory<std::string, Play, ShootOrPassPlay, AiConfig> factory;
