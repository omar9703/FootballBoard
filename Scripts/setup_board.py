import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
manager_class = unreal.load_class(None, '/Script/FootballBoard.BoardManager')
assert manager_class, 'BoardManager module missing'
mesh0 = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
mesh1 = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple')
anim = unreal.load_asset('/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd')
assert mesh0 and mesh1 and anim, 'Runner assets missing'
for role in ('Sender', 'Receiver'):
    path = '/Game/FootballBoard/Level_' + role
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        assert levels.load_level(path)
        assert not any(a.get_class() == manager_class for a in actors.get_all_level_actors()), 'Level already configured'
    else:
        assert levels.new_level(path)
    light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(-60, -30, 0))
    light.set_actor_label('Board Sun')
    light.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(3.0)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    manager = actors.spawn_actor_from_class(manager_class, unreal.Vector(0, 0, 0))
    manager.set_actor_label('BoardManager_' + role)
    manager.set_editor_property('bIsSender', role == 'Sender')
    manager.set_editor_property('RunnerMeshTeam0', mesh0)
    manager.set_editor_property('RunnerMeshTeam1', mesh1)
    manager.set_editor_property('RunnerRunAnim', anim)
    assert levels.save_current_level()
levels.load_level('/Game/FootballBoard/Level_Sender')
unreal.log('FOOTBALL_SETUP_COMPLETE')
