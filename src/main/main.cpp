#include "version.h"
#include "window.h"
#include "render/i_render.h"
#include "platform/i_platform.h"
#include "input/i_input.h"
#include "audio/i_audio.h"
#include "core/scene.h"
#include "platform/backtrace.h"

#include <boost/timer.hpp>
#include "engine/engine.h"
#include "engine/collision_system.h"
#include "engine/controllers/controller_system.h"
#include "core/opt.h"
#include "engine/items/inventory_system.h"
#include "core/item.h"
#include "engine/items/weapon_item_sub_system.h"
#include "platform/event.h"
#include "render/recognizer_repo.h"
#include <boost/program_options.hpp>
#include "core/program_state.h"
#include "network/message_handler_sub_system_holder.h"
#include <stdlib.h>
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/date_time/posix_time/posix_time_config.hpp"
#include "boost/date_time/gregorian/greg_date.hpp"
#include "engine/items/normal_item_sub_system.h"
#include "engine/buffs_engine/buff_holder_system.h"
#include "core/buffs/heal_over_time_buff.h"
#include "core/buffs/max_health_buff.h"
#include "core/buffs/accuracy_buff.h"
#include "render/damage_particles.h"
#include "render/corpses.h"
#include "network/shot_message.h"
#include "network/kill_score_message.h"
#include "network/client_score_message.h"
#include "core/free_for_all_game_mode_system.h"
#include "core/capture_the_flag_game_mode_system.h"
#include "render/particle_system.h"
#include "network/server_system.h"
#include "network/client_system.h"
#include "core/buffs/armor_buff.h"
#include "core/buffs/cloak_buff.h"
#include "audio/audio_system.h"
#include "audio/audio_effect_repo.h"
#include "platform/init.h"
#include <portable_iarchive.hpp>
#include <portable_oarchive.hpp>
#include <iosfwd>
#include "core/component_factory.h"
#include "network/actor_list_message.h"
#include "engine/remove_components_on_death_system.h"
#include "core/buffs/move_speed_buff.h"
#include "network/client_datas_message.h"
#include "network/ctf_client_datas_message.h"
#include "network/soldier_properties_message.h"
#include "input/player_control_device.h"
#include "input/controller_adapter_system.h"

using engine::Engine;
namespace {
class Timer_t
{
    boost::timer mMeasurer;
    double mPrevMeasurement;
public:
    void Log( std::string const& Str = std::string() )
    {
        double Now = mMeasurer.elapsed();
        L2( "Timer - %s: %f %f\n", Str.c_str(), Now, Now - mPrevMeasurement );
        mPrevMeasurement = Now;
    }
    Timer_t(): mMeasurer(), mPrevMeasurement( mMeasurer.elapsed() )
    {
        Log( "timer init" );
    }
} PerfTimer;

}

static bool IsMainRunning;

void OnPhaseChangedEvent( PhaseChangedEvent const& Evt )
{
    if( Evt.CurrentPhase == ProgramPhase::InitiateShutDown )
    {
        IsMainRunning = false;
    }
}
#include "network/message_order.h"

void ForceReceiveSendMessages()
{
    Engine& Eng = Engine::Get();
    Opt<network::ServerSystem> serverSystem( Eng.GetSystem<network::ServerSystem>() );
    Opt<network::ClientSystem> clientSystem( Eng.GetSystem<network::ClientSystem>() );
    if ( clientSystem.IsValid() )
    {
        clientSystem->Update( 0.0 );
    }
    if ( serverSystem.IsValid() )
    {
        serverSystem->Update( 0.0 );
    }

    Opt<network::MessageHandlerSubSystemHolder> messageHandlerSSH( Eng.GetSystem<network::MessageHandlerSubSystemHolder>() );
    if ( messageHandlerSSH.IsValid() )
    {
        messageHandlerSSH->Update( 0.0 );
    }

    if ( clientSystem.IsValid() )
    {
        clientSystem->Update( 0.0 );
    }
    if ( serverSystem.IsValid() )
    {
        serverSystem->Update( 0.0 );
    }
}

int main( int argc, char* argv[] )
{
    boost::posix_time::ptime t = boost::posix_time::microsec_clock::local_time();

    // convert to int64_t
    boost::posix_time::ptime myEpoch( boost::gregorian::date( 1970, boost::gregorian::Jan, 1 ) );
    boost::posix_time::time_duration myTimeFromEpoch = t - myEpoch;
    srand ( int32_t( myTimeFromEpoch.total_milliseconds() ) );
    using core::ProgramState;
    ProgramState& programState = ProgramState::Get();
    namespace po = boost::program_options;
    po::options_description desc( "Allowed options" );
    std::string deviceConfig;
    desc.add_options()
    ( "help", "produce help message" )
    ( "-c", po::value<std::string>( &programState.mServerIp ), "client" )
    ( "-s", "server ip" )
    ( "-n", po::value<std::string>( &programState.mClientName ), "client name" )
    ( "-v", "print version information" )
    ( "-h", "connect as a client to localhost with Host privileges" )
    ( "-r", "connect as a random named soldier to localhost." )
    ( "-d", po::value<std::string>( &deviceConfig ), "set device configuration, format: player1:controller:1,player2:keyboard_and_mouse" )
    ( "calibrate", "print values read from detected controllers" )
    ;

    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );
    if ( vm.count( "help" ) )
    {
        std::cout << desc << "\n";
        return 1;
    }
    else if( vm.count( "-v" ) )
    {
        std::cout << GIT_VERSION << "\n"
                  << GIT_DATE << "\n"
                  << GIT_BRANCH << "\n"
                  << GIT_REMOTE << "\n";
        return 0;
    }
    if ( vm.count( "-c" ) )
    {
        L1( "run as client" );
        programState.SetMode( ProgramState::Client );
    }
    else if ( vm.count( "-s" ) )
    {
        L1( "run as server" );
        programState.SetMode( ProgramState::Server );
    }
    else if ( vm.count( "-h" ) )
    {
        L1( "run as host" );
        programState.SetMode( ProgramState::Client );
        programState.mIsHost = 1;
        programState.mServerIp = "localhost";
    }
    else
    {
        L1( "run local" );
        programState.SetMode( ProgramState::Local );
    }
    if ( vm.count( "-r" ) )
    {
        L1( "run as a random named soldier. RanBro" );
        programState.SetMode( ProgramState::Client );
        programState.mServerIp = "localhost";
        programState.mClientName = "RanBro" + boost::lexical_cast<std::string>( rand() % 1000 );
    }
    bool calibrateController = vm.count( "calibrate" );
    Filesys::Get().Mount( std::auto_ptr<Package>( new Package( AutoFile( new OsFile( "data.pkg" ) ) ) ) );
    platform::IdStorage::Get().Init();
    platform::Init::Get().Execute();
    IsMainRunning = true;
    input::PlayerControlDevice::Get().SetControlDeviceConfiguration( deviceConfig );
    EventServer<PhaseChangedEvent>& PhaseChangeEventServer( EventServer<PhaseChangedEvent>::Get() );
    AutoReg PhaseChangeId( PhaseChangeEventServer.Subscribe( &OnPhaseChangedEvent ) );

    Engine& Eng = Engine::Get();

    Eng.AddSystem( AutoId( "window_system" ) );
    if( !Eng.GetSystem<engine::WindowSystem>()->Create( 640, 480, "Reaping2" ) )
    {
        PhaseChangeEventServer.SendEvent( PhaseChangedEvent( ProgramPhase::InitiateShutDown ) );
    }


    RootModel::Get();
    PhaseChangeEventServer.SendEvent( PhaseChangedEvent( ProgramPhase::Startup ) );
    PerfTimer.Log( "wnd" );
    AudioPlayer::Get();
    audio::AudioEffectRepo::Get();
    DamageDecals::Get();
    Corpses::Get();
    PerfTimer.Log( "renderer" );
    Scene& Scen = Scene::Get();
    PerfTimer.Log( "scene" );
    render::RecognizerRepo::Get();

    Eng.AddSystem( AutoId( "free_for_all_game_mode_system" ) );
    Eng.AddSystem( AutoId( "capture_the_flag_game_mode_system" ) );
    Eng.AddSystem( AutoId( "leaderboard_system" ) );
    ::engine::Engine::Get().SetEnabled< ::core::FreeForAllGameModeSystem>( false );
    ::engine::Engine::Get().SetEnabled< ::core::CaptureTheFlagGameModeSystem>( false );

    if ( programState.mMode != ProgramState::Local )
    {
        Eng.AddSystem( AutoId( "position_message_sender_system" ) );
        Eng.AddSystem( AutoId( "move_message_sender_system" ) );
        Eng.AddSystem( AutoId( "orientation_message_sender_system" ) );
        Eng.AddSystem( AutoId( "heading_message_sender_system" ) );
    }
    if ( programState.mMode == ProgramState::Server )
    {
        Eng.AddSystem( AutoId( "server_system" ) );
        Eng.AddSystem( AutoId( "create_actor_message_sender_system" ) );
        Eng.AddSystem( AutoId( "heal_taken_message_sender_system" ) );
        Eng.AddSystem( AutoId( "damage_taken_message_sender_system" ) );
        Eng.AddSystem( AutoId( "pickup_message_sender_system" ) );
        Eng.AddSystem( AutoId( "flash_message_sender_system" ) );
        Eng.AddSystem( AutoId( "set_ownership_message_sender_system" ) );
        Eng.AddSystem( AutoId( "set_team_message_sender_system" ) );
        Eng.AddSystem( AutoId( "ctf_score_message_sender_system" ) );
        Eng.AddSystem( AutoId( "kill_score_message_sender_system" ) );
        Eng.AddSystem( AutoId( "show_text_message_message_sender_system" ) );
        Eng.AddSystem( AutoId( "collision_message_sender_system" ) );
        Eng.AddSystem( AutoId( "health_message_sender_system" ) );
        Eng.AddSystem( AutoId( "shot_message_sender_system" ) );
        Eng.AddSystem( AutoId( "client_datas_message_sender_system" ) );
        Eng.AddSystem( AutoId( "client_score_message_sender_system" ) );
        Eng.AddSystem( AutoId( "item_changed_message_sender_system" ) );
        Eng.AddSystem( AutoId( "cloak_changed_message_sender_system" ) );
        Eng.AddSystem( AutoId( "border_message_sender_system" ) );
        Eng.AddSystem( AutoId( "sync_item_message_sender_system" ) );
        Eng.AddSystem( AutoId( "secs_to_revive_message_sender_system" ) );
        Eng.AddSystem( AutoId( "modify_audible_component_message_sender_system" ) );
        Eng.AddSystem( AutoId( "flag_state_changed_message_sender_system" ) );
        Eng.AddSystem( AutoId( "fade_out_message_sender_system" ) );
        Eng.AddSystem( AutoId( "ctf_client_datas_message_sender_system" ) );
        Eng.AddSystem( AutoId( "rotate_message_sender_system" ) );

        Eng.AddSystem( AutoId( "actor_list_message_sender_system" ) );
        Eng.AddSystem( AutoId( "data_checksum_message_sender_system" ) );
    }
    if (programState.mMode != ProgramState::Client)
    {
        Eng.AddSystem( AutoId( "ctf_client_list_handling_system" ) );
    }
    if (programState.mMode != ProgramState::Server)
    {
        Eng.AddSystem( AutoId( "ctf_client_list_displaying_system" ) );
        Eng.AddSystem( AutoId( "ffa_client_list_system" ) );
    }
    if ( programState.mMode == ProgramState::Client )
    {
        Eng.AddSystem( AutoId( "client_system" ) );
        Eng.AddSystem( AutoId( "lifecycle_sender_system" ) );
        Eng.AddSystem( AutoId( "player_controller_message_sender_system" ) );
        Eng.AddSystem( AutoId( "ping_message_sender_system" ) );
        Eng.AddSystem( AutoId( "revive_message_sender_system" ) );
        Eng.AddSystem( AutoId( "gamemode_selected_message_sender_system" ) );
        Eng.AddSystem( AutoId( "waiting_system" ) );
        Eng.AddSystem( AutoId( "team_switch_request_message_sender_system" ) );

    }
    if ( programState.mMode == ProgramState::Local )
    {
        Eng.AddSystem( AutoId( "local_system" ) );
        Eng.AddSystem( AutoId( "editor_system" ) );

        Eng.AddSystem( AutoId( "editor_target_system" ) );
        Eng.AddSystem( AutoId( "editor_grid_system" ) );
        Eng.AddSystem( AutoId( "editor_brush_system" ) );
        Eng.AddSystem( AutoId( "editor_soldier_spawn_system" ) );
    }

    if ( programState.mMode != ProgramState::Client )
    {
        Eng.AddSystem( AutoId( "map_system" ) );
        Eng.AddSystem( AutoId( "link_map_element_system" ) );
        Eng.AddSystem( AutoId( "map_start_map_element_system" ) );
        Eng.AddSystem( AutoId( "spawn_soldiers_map_element_system" ) );
        Eng.AddSystem( AutoId( "soldier_spawn_point_map_element_system" ) );
        Eng.AddSystem( AutoId( "spawn_actor_map_element_system" ) );
        Eng.AddSystem( AutoId( "ctf_soldier_spawn_point_map_element_system" ) );
        Eng.AddSystem( AutoId( "ctf_spawn_soldiers_map_element_system" ) );
        Eng.AddSystem( AutoId( "ctf_spawn_flags_map_element_system" ) );
        Eng.AddSystem( AutoId( "respawn_actor_map_element_system" ) );
        Eng.AddSystem( AutoId( "soldier_auto_revive_map_element_system" ) );
    }

    Eng.AddSystem( AutoId( "soldier_properties_system" ) ); //must be before message_sender
    Eng.AddSystem( AutoId( "soldier_spawn_system" ) );
    Eng.AddSystem( AutoId( "flag_spawn_system" ) );
    Eng.AddSystem( AutoId( "ctf_soldier_spawn_system" ) );
    if ( programState.mMode != ProgramState::Local )
    {
        Eng.AddSystem( AutoId( "message_handler_sub_system_holder" ) );
        Eng.AddSystem( AutoId( "soldier_properties_message_sender_system" ) );

        Opt<network::MessageHandlerSubSystemHolder> messageHandlerSSH( Eng.GetSystem<network::MessageHandlerSubSystemHolder>() );
        messageHandlerSSH->InitHandlers();
    }

    Eng.AddSystem( AutoId( "timer_server_system" ) );
    if ( programState.mMode != ProgramState::Server )
    {
        Eng.AddSystem( AutoId( "keyboard_system" ) );
        Eng.AddSystem( AutoId( "mouse_system" ) );
        Eng.AddSystem( AutoId( "input_system" ) );
        //adapter systems should be here. after input system before controller systems.
        Eng.AddSystem( AutoId( "keyboard_adapter_system" ) );
        Eng.AddSystem( AutoId( "mouse_adapter_system" ) );
        Eng.AddSystem( AutoId( "controller_adapter_system" ) );
        Opt<engine::ControllerAdapterSystem> cntrlAdapter( Eng.GetSystem<engine::ControllerAdapterSystem>() );
        cntrlAdapter->SetCalibrate( calibrateController );
    }
    Eng.AddSystem( AutoId( "buff_holder_system" ) );
    Opt<engine::BuffHolderSystem> buffHolderS( Eng.GetSystem<engine::BuffHolderSystem>() );
    buffHolderS->AddSubSystem( MoveSpeedBuff::GetType_static(), AutoId( "move_speed_buff_sub_system" ) );
    if ( programState.mMode != ProgramState::Client )
    {
        Eng.AddSystem( AutoId( "detonate_on_hit_system" ) );
        Eng.AddSystem( AutoId( "explode_on_hit_system" ) );

        buffHolderS->AddSubSystem( HealOverTimeBuff::GetType_static(), AutoId( "heal_over_time_buff_sub_system" ) );
        buffHolderS->AddSubSystem( MaxHealthBuff::GetType_static(), AutoId( "max_health_buff_sub_system" ) );
        buffHolderS->AddSubSystem( AccuracyBuff::GetType_static(), AutoId( "accuracy_buff_sub_system" ) );
        buffHolderS->AddSubSystem( CloakBuff::GetType_static(), AutoId( "cloak_buff_sub_system" ) );
    }
    buffHolderS->AddSubSystem( ArmorBuff::GetType_static(), AutoId( "armor_buff_sub_system" ) );

    Eng.AddSystem( AutoId( "controller_system" ) );
    Opt<engine::ControllerSystem> controllserSystem( Eng.GetSystem<engine::ControllerSystem>() );
    controllserSystem->AddSubSystem( AutoId( "player_controller_component" ), AutoId( "player_controller_sub_system" ) );
    controllserSystem->AddSubSystem( AutoId( "random_controller_component" ), AutoId( "random_controller_sub_system" ) );
    controllserSystem->AddSubSystem( AutoId( "target_player_controller_component" ), AutoId( "target_player_controller_sub_system" ) );
    controllserSystem->AddSubSystem( AutoId( "pointer_target_controller_component" ), AutoId( "pointer_target_controller_sub_system" ) );

    Eng.AddSystem( AutoId( "cloak_system" ) );

    Eng.AddSystem( AutoId( "inventory_system" ) );
    Opt<engine::InventorySystem> inventorySystem( Eng.GetSystem<engine::InventorySystem>() );
    inventorySystem->AddSubSystem( ItemType::Weapon, AutoId( "weapon_item_sub_system" ) );
    Opt<engine::WeaponItemSubSystem> weaponItemSS = engine::WeaponItemSubSystem::Get();
    if ( programState.mMode != ProgramState::Client )
    {
        weaponItemSS->AddSubSystem( AutoId( "plasma_gun" ), AutoId( "plasma_gun_weapon_sub_system" ) );
        weaponItemSS->AddSubSystem( AutoId( "pistol" ), AutoId( "pistol_weapon_sub_system" ) );
        weaponItemSS->AddSubSystem( AutoId( "shotgun" ), AutoId( "shotgun_weapon_sub_system" ) );
        weaponItemSS->AddSubSystem( AutoId( "rocket_launcher" ), AutoId( "rocket_launcher_weapon_sub_system" ) );
        weaponItemSS->AddSubSystem( AutoId( "ion_gun" ), AutoId( "ion_gun_weapon_sub_system" ) );
        weaponItemSS->AddSubSystem( AutoId( "lucky_rocket" ), AutoId( "lucky_rocket_weapon_sub_system" ) );
        weaponItemSS->AddSubSystem( AutoId( "rusty_reaper" ), AutoId( "rusty_reaper_weapon_sub_system" ) );

        inventorySystem->AddSubSystem( ItemType::Normal, AutoId( "normal_item_sub_system" ) );
        Opt<engine::NormalItemSubSystem> normalItemSS = engine::NormalItemSubSystem::Get();
        normalItemSS->AddSubSystem( AutoId( "grenade_normal_item" ), AutoId( "grenade_normal_item_sub_system" ) );
        normalItemSS->AddSubSystem( AutoId( "blue_grenade_normal_item" ), AutoId( "blue_grenade_normal_item_sub_system" ) );
        normalItemSS->AddSubSystem( AutoId( "flash_normal_item" ), AutoId( "flash_normal_item_sub_system" ) );
        normalItemSS->AddSubSystem( AutoId( "cloak_normal_item" ), AutoId( "cloak_normal_item_sub_system" ) );
    }
    weaponItemSS->AddSubSystem( AutoId( "gauss_gun" ), AutoId( "gauss_gun_weapon_sub_system" ) );
    weaponItemSS->AddSubSystem( AutoId( "gatling_gun" ), AutoId( "gatling_gun_weapon_sub_system" ) ); //handles client specific stuff like windup and deploy states.
    Eng.AddSystem( AutoId( "audio_system" ) );
    Eng.AddSystem( AutoId( "fade_out_system" ) );
    if ( programState.mMode != ProgramState::Client )
    {
        // these must be before health_system
        Eng.AddSystem( AutoId( "drop_on_death_system" ) );
        Eng.AddSystem( AutoId( "target_holder_system" ) );
        Eng.AddSystem( AutoId( "score_on_death_system" ) );
        Eng.AddSystem( AutoId( "kill_score_on_death_system" ) );
    }
    Eng.AddSystem( AutoId( "armor_system" ) ); //must be before health_system (lowers damage income)
    Eng.AddSystem( AutoId( "health_system" ) );

    if ( programState.mMode != ProgramState::Client )
    {
        Eng.AddSystem( AutoId( "notify_parent_on_death_system" ) );
        Eng.AddSystem( AutoId( "listen_child_death_system" ) );
        Eng.AddSystem( AutoId( "remove_on_death_system" ) );
        Eng.AddSystem( AutoId( "explode_on_death_system" ) );
    }
    Eng.AddSystem( AutoId( "remove_components_on_death_system" ) );
    Eng.AddSystem( AutoId( "soldier_auto_revive_system" ) );
    Eng.AddSystem( AutoId( "explosion_system" ) );
    Eng.AddSystem( AutoId( "acceleration_system" ) );
    Eng.AddSystem( AutoId( "collision_system" ) );
    Opt<engine::CollisionSystem> collisionSS( Eng.GetSystem<engine::CollisionSystem>() );
    collisionSS->AddSubSystem( AutoId( "wall_collision_component" ), AutoId( "wall_collision_sub_system" ) );
    collisionSS->AddSubSystem( AutoId( "water_collision_component" ), AutoId( "wall_collision_sub_system" ) );
    collisionSS->AddSubSystem( AutoId( "collision_component" ), AutoId( "normal_collision_sub_system" ) );
    collisionSS->AddSubSystem( AutoId( "bounce_collision_component" ), AutoId( "bounce_collision_sub_system" ) );
    if ( programState.mMode != ProgramState::Client )
    {
        collisionSS->AddSubSystem( AutoId( "pickup_collision_component" ), AutoId( "pickup_collision_sub_system" ) );
        collisionSS->AddSubSystem( AutoId( "shot_collision_component" ), AutoId( "shot_collision_sub_system" ) );
        collisionSS->AddSubSystem( AutoId( "aoe_collision_component" ), AutoId( "aoe_collision_sub_system" ) );
        collisionSS->AddSubSystem( AutoId( "flag_collision_component" ), AutoId( "flag_collision_sub_system" ) );
    }
    Eng.AddSystem( AutoId( "ParticleSystem" ) );
    Eng.AddSystem( AutoId( "move_system" ) );
    Eng.AddSystem( AutoId( "rotate_component_system" ) );
    Eng.AddSystem( AutoId( "attachable_system" ) );

    Eng.AddSystem( AutoId( "kill_score_system" ) );

    Eng.AddSystem( AutoId( "stop_on_death_system" ) );

    Eng.AddSystem( AutoId( "frame_counter_system" ) );
    if ( programState.mMode != ProgramState::Server )
    {
        Eng.AddSystem( AutoId( "renderer_system" ) );
    }
    Eng.AddSystem( AutoId( "show_text_system" ) );

    Eng.AddSystem( AutoId( "player_model_system" ) );
    if ( programState.mMode != ProgramState::Client )
    {
        Eng.AddSystem( AutoId( "removed_actors_system" ) );
    }
    Eng.Init();
    Eng.SetEnabled<engine::CollisionSystem>( true ); //just for testing
    Eng.SetEnabled<render::ParticleSystem>( true );

    static const double MaxFrameRate = 60.;
    static const double MinFrameTime = 1. / MaxFrameRate;
    double Prevtime, Curtime;
    Prevtime = Curtime = glfwGetTime();
    PhaseChangeEventServer.SendEvent( PhaseChangedEvent( ProgramPhase::Running ) );
    EventServer<CycleEvent>& CycleEventServer( EventServer<CycleEvent>::Get() );

    L1( "ctf_client_datas_message type: %d\n", network::ctf::ClientDatasMessage::GetType_static() );
    L1( "client_datas_message type: %d\n", network::ClientDatasMessage::GetType_static() );
    L1( "soldier_properties_message type: %d\n", network::SoldierPropertiesMessage::GetType_static() );

    while( IsMainRunning )
    {
        Curtime = glfwGetTime();
        double Dt = Curtime - Prevtime;
        if( Dt < MinFrameTime )
        {
            const double SleepTime = ( MinFrameTime - Dt );
            boost::this_thread::sleep( boost::posix_time::milliseconds( boost::int64_t( SleepTime * 500. ) ) );
            ForceReceiveSendMessages();
            double Curtime2 = glfwGetTime();
            double Dt2 = Curtime2 - Curtime;
            const double SleepTime2 = ( SleepTime - Dt2 );
            boost::this_thread::sleep( boost::posix_time::milliseconds( boost::int64_t( SleepTime2 * 1000. ) ) );
            ForceReceiveSendMessages();
            Dt = MinFrameTime;
            Curtime = glfwGetTime();
        }
        PerfTimer.Log( "Frame started" );
        Eng.Update( Dt );
        Scen.Update( Dt );
        CycleEventServer.SendEvent( CycleEvent( Curtime ) );

        Prevtime = Curtime;
        PerfTimer.Log( "Frame ended" );
    }
    PhaseChangeEventServer.SendEvent( PhaseChangedEvent( ProgramPhase::CloseSignal ) );
    PhaseChangeEventServer.SendEvent( PhaseChangedEvent( ProgramPhase::Shutdown ) );
    return 0;
}
