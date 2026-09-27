import os
import traceback
import unreal

DESTINATION = "/Game/Environment/Industrial"
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "ArtSource", "Industrial")
CONTENT_DIR = os.path.join(PROJECT_DIR, "Content", "Environment", "Industrial")

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
    unreal.log("[UltraRealFPS Art Import] " + str(message))

def fail(message):
    unreal.log_error("[UltraRealFPS Art Import] " + str(message))
    raise RuntimeError(message)

def package_path(obj):
    path = obj.get_path_name()
    return path.rsplit(".", 1)[0] if "." in path else path

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

def get_static_mesh_editor_subsystem():
    try:
        return unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    except Exception as exc:
        unreal.log_warning("[UltraRealFPS Art Import] StaticMeshEditorSubsystem indisponible: %s" % exc)
        return None

def add_collision(static_mesh, asset_name, subsystem):
    if asset_name not in COLLISION_BOX_ASSETS or not static_mesh:
        return
    try:
        shape_enum = getattr(unreal, "ScriptCollisionShapeType", None)
        if shape_enum is None:
            shape_enum = unreal.ScriptingCollisionShapeType

        if subsystem:
            subsystem.remove_collisions(static_mesh)
            result = subsystem.add_simple_collisions(static_mesh, shape_enum.BOX)
            if result < 0:
                unreal.log_warning(
                    "[UltraRealFPS Art Import] Collision box non ajoutee sur %s (code %s)"
                    % (asset_name, result)
                )
        else:
            unreal.EditorStaticMeshLibrary.remove_collisions(static_mesh)
            unreal.EditorStaticMeshLibrary.add_simple_collisions(static_mesh, shape_enum.BOX)

        unreal.EditorAssetLibrary.save_loaded_asset(static_mesh, False)
    except Exception as exc:
        unreal.log_warning(
            "[UltraRealFPS Art Import] Collision simple non appliquee sur %s: %s"
            % (asset_name, exc)
        )

def task_meshes(task):
    found = []

    try:
        for obj in task.get_objects():
            if isinstance(obj, unreal.StaticMesh):
                found.append(obj)
    except Exception as exc:
        unreal.log_warning("[UltraRealFPS Art Import] get_objects indisponible: %s" % exc)

    try:
        for path in task.get_editor_property("imported_object_paths"):
            log("Objet importe: " + path)
            obj = unreal.EditorAssetLibrary.load_asset(path)
            if isinstance(obj, unreal.StaticMesh) and obj not in found:
                found.append(obj)
    except Exception as exc:
        unreal.log_warning("[UltraRealFPS Art Import] Lecture imported_object_paths impossible: %s" % exc)

    return found

def find_mesh_by_name(expected_name):
    try:
        for path in unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True, include_folder=False):
            obj = unreal.EditorAssetLibrary.load_asset(path)
            if isinstance(obj, unreal.StaticMesh) and obj.get_name() == expected_name:
                return obj
    except Exception as exc:
        unreal.log_warning("[UltraRealFPS Art Import] Scan du dossier impossible: %s" % exc)
    return None

def canonicalize_mesh(mesh, expected_name):
    target_package = DESTINATION + "/" + expected_name
    current_package = package_path(mesh)

    if current_package != target_package:
        existing = None
        if unreal.EditorAssetLibrary.does_asset_exist(target_package):
            existing = unreal.EditorAssetLibrary.load_asset(target_package)

        if isinstance(existing, unreal.StaticMesh):
            log("Asset canonique deja present: " + target_package)
            mesh = existing
        else:
            log("Normalisation asset: %s -> %s" % (current_package, target_package))
            if not unreal.EditorAssetLibrary.rename_loaded_asset(mesh, target_package):
                fail("Impossible de normaliser %s vers %s" % (current_package, target_package))
            mesh = unreal.EditorAssetLibrary.load_asset(target_package) or mesh

    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
    return mesh

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

    static_mesh_subsystem = get_static_mesh_editor_subsystem()
    imported_meshes = []

    for source_file, task in zip(files, tasks):
        expected_name = os.path.splitext(os.path.basename(source_file))[0]
        meshes = task_meshes(task)

        mesh = None
        for candidate in meshes:
            if candidate.get_name() == expected_name:
                mesh = candidate
                break
        if mesh is None and meshes:
            mesh = meshes[0]
        if mesh is None:
            mesh = find_mesh_by_name(expected_name)
        if mesh is None:
            fail("Aucun StaticMesh trouve apres import de %s" % os.path.basename(source_file))

        mesh = canonicalize_mesh(mesh, expected_name)
        add_collision(mesh, expected_name, static_mesh_subsystem)
        imported_meshes.append(mesh)
        log("OK mesh: %s" % mesh.get_path_name())

    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)

    expected_names = [os.path.splitext(os.path.basename(path))[0] for path in files]
    missing = []
    for expected_name in expected_names:
        target_package = DESTINATION + "/" + expected_name
        disk_path = os.path.join(CONTENT_DIR, expected_name + ".uasset")
        asset_exists = unreal.EditorAssetLibrary.does_asset_exist(target_package)
        file_exists = os.path.isfile(disk_path)
        if not asset_exists and not file_exists:
            missing.append("%s (asset=%s, fichier=%s)" % (expected_name, target_package, disk_path))

    if missing:
        fail("Import incomplet. Assets manquants:\n- " + "\n- ".join(missing))

    log("Import termine avec succes: %d StaticMesh valides." % len(imported_meshes))
    log("Sentinelle OK: " + os.path.join(CONTENT_DIR, "SM_Container20_A.uasset"))

try:
    main()
except Exception:
    unreal.log_error("[UltraRealFPS Art Import] " + traceback.format_exc())
    raise
