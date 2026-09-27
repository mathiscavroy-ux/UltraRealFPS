import os
import sys
import traceback
import unreal

DESTINATION = "/Game/Environment/Industrial"
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "ArtSource", "Industrial")

COLLISION_BOX_ASSETS = {
    "SM_Container20_A",
    "SM_ConcreteBarrier_A",
    "SM_IndustrialCrate_A",
    "SM_Pallet_A",
    "SM_ElectricalCabinet_A",
    "SM_FuelTank_A",
    "SM_StorageRack_A",
}

def log(message):
    unreal.log("[UltraRealFPS Art Import] " + message)

def fail(message):
    unreal.log_error("[UltraRealFPS Art Import] " + message)
    raise RuntimeError(message)

def make_task(filename):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    try:
        task.set_editor_property("async_", False)
    except Exception:
        pass
    return task

def add_collision(static_mesh, asset_name):
    if asset_name not in COLLISION_BOX_ASSETS:
        return
    try:
        unreal.EditorStaticMeshLibrary.remove_collisions(static_mesh)
        unreal.EditorStaticMeshLibrary.add_simple_collisions(
            static_mesh,
            unreal.ScriptingCollisionShapeType.BOX
        )
        unreal.EditorAssetLibrary.save_loaded_asset(static_mesh)
    except Exception as exc:
        unreal.log_warning(
            "[UltraRealFPS Art Import] Collision simple non appliquee sur %s: %s"
            % (asset_name, exc)
        )

def main():
    if not os.path.isdir(SOURCE_DIR):
        fail("Dossier source introuvable: " + SOURCE_DIR)

    files = sorted(
        os.path.join(SOURCE_DIR, name)
        for name in os.listdir(SOURCE_DIR)
        if name.lower().endswith((".glb", ".gltf", ".fbx"))
    )
    if not files:
        fail("Aucun modele 3D trouve dans " + SOURCE_DIR)

    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
        unreal.EditorAssetLibrary.make_directory(DESTINATION)

    tasks = [make_task(filename) for filename in files]
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    log("Import de %d modeles vers %s" % (len(tasks), DESTINATION))
    asset_tools.import_asset_tasks(tasks)

    imported_paths = []
    for task in tasks:
        for path in task.get_editor_property("imported_object_paths"):
            imported_paths.append(path)

    # Apply deliberately simple collision only to props where a box approximation is safe.
    for asset_path in unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            add_collision(asset, asset.get_name())

    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)

    expected = DESTINATION + "/SM_Container20_A.SM_Container20_A"
    if not unreal.EditorAssetLibrary.does_asset_exist(expected):
        fail("Import termine mais l'asset sentinelle manque: " + expected)

    log("Import termine. Assets crees/mis a jour: %d" % len(imported_paths))
    log("Sentinelle OK: " + expected)

try:
    main()
except Exception:
    fail(traceback.format_exc())
