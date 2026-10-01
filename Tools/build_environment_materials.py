"""UltraRealFPS - generates the environment master material (MATERIAL_PIPELINE.md, step 3).

Run through BUILD_MATERIALS.bat:
    UnrealEditor-Cmd.exe UltraRealFPS.uproject -run=pythonscript -script=Tools/build_environment_materials.py

It (re)builds /Game/UltraRealFPS/Art/Materials/Environment/Industrial/M_Master_IndustrialSurface.
AURFPSGameMode loads that material when it exists and creates one shared instance per blockout
style (BaseColorTint, Roughness, Metallic, DirtAmount, MacroVariation, MacroScale come from the
C++ palette). When the asset is missing the game keeps its BasicShapeMaterial palette, so any
failure here deletes the half-built asset instead of leaving a broken material in the project.

Graph:
    BaseColor = lerp(Tint * Breakup, Tint * Breakup * DirtTint, DirtMask)
    Roughness = saturate(Roughness + DirtMask * 0.12)
    Metallic  = Metallic
    Breakup   = lerp(1 - MacroVariation, 1 + MacroVariation, Noise(WorldPosition / MacroScale))
    DirtMask  = (1 - saturate((WorldZ - GroundHeight) / DirtHeight)) * DirtAmount
World-space breakup needs no UVs, which the engine blockout cubes do not provide consistently.
"""
import traceback

import unreal

FOLDER = "/Game/UltraRealFPS/Art/Materials/Environment/Industrial"
MASTER_NAME = "M_Master_IndustrialSurface"
MASTER_PATH = FOLDER + "/" + MASTER_NAME

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def log(message):
    unreal.log("[UltraRealFPS Materials] " + str(message))


def warn(message):
    unreal.log_warning("[UltraRealFPS Materials] " + str(message))


def fail(message):
    unreal.log_error("[UltraRealFPS Materials] " + str(message))
    raise RuntimeError(message)


def get_or_create_master():
    if EAL.does_asset_exist(MASTER_PATH):
        material = EAL.load_asset(MASTER_PATH)
        if not isinstance(material, unreal.Material):
            fail("%s existe mais n'est pas un Material" % MASTER_PATH)
        log("Reconstruction du graphe de " + MASTER_PATH)
        MEL.delete_all_material_expressions(material)
        return material

    if not EAL.does_directory_exist(FOLDER):
        EAL.make_directory(FOLDER)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(MASTER_NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(material, unreal.Material):
        fail("Creation impossible: " + MASTER_PATH)
    log("Creation de " + MASTER_PATH)
    return material


class Graph(object):
    def __init__(self, material):
        self.material = material

    def node(self, expression_class, x, y, **properties):
        expression = MEL.create_material_expression(self.material, expression_class, x, y)
        if expression is None:
            fail("Expression non creee: %s" % expression_class.__name__)
        for name, value in properties.items():
            expression.set_editor_property(name, value)
        return expression

    def optional(self, expression, name, value):
        # Cosmetic tuning only: an unknown property name must not abort the whole build.
        try:
            expression.set_editor_property(name, value)
        except Exception as exc:
            warn("Propriete %s ignoree sur %s: %s" % (name, expression.get_name(), exc))

    def scalar(self, name, default, x, y, group):
        return self.node(unreal.MaterialExpressionScalarParameter, x, y,
                         parameter_name=name, default_value=float(default), group=group)

    def vector(self, name, rgb, x, y, group):
        return self.node(unreal.MaterialExpressionVectorParameter, x, y,
                         parameter_name=name, default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0),
                         group=group)

    def link(self, source, target, target_input=""):
        if not MEL.connect_material_expressions(source, "", target, target_input):
            fail("Connexion impossible: %s -> %s.%s" % (source.get_name(), target.get_name(), target_input))

    def output(self, source, material_property):
        if not MEL.connect_material_property(source, "", material_property):
            fail("Connexion impossible vers %s" % material_property)


def build_graph(material):
    g = Graph(material)

    # Parameters (names are read by AURFPSGameMode::CreateMaterials).
    base_tint = g.vector("BaseColorTint", (0.150, 0.155, 0.160), -1700, -520, "Surface")
    roughness = g.scalar("Roughness", 0.85, -700, 240, "Surface")
    metallic = g.scalar("Metallic", 0.0, -700, 380, "Surface")
    dirt_tint = g.vector("DirtTint", (0.55, 0.50, 0.44), -1700, -300, "Dirt")
    dirt_amount = g.scalar("DirtAmount", 0.35, -1700, 120, "Dirt")
    dirt_height = g.scalar("DirtHeight", 90.0, -1700, 220, "Dirt")
    ground_height = g.scalar("GroundHeight", -100.0, -1700, 320, "Dirt")
    macro_variation = g.scalar("MacroVariation", 0.12, -1700, 520, "Breakup")
    macro_scale = g.scalar("MacroScale", 400.0, -1700, 620, "Breakup")

    world_position = g.node(unreal.MaterialExpressionWorldPosition, -2000, 0)

    # Damp / dirt band at the foot of every surface standing on the slab.
    world_z = g.node(unreal.MaterialExpressionComponentMask, -1450, 0, r=False, g=False, b=True, a=False)
    g.link(world_position, world_z)
    height = g.node(unreal.MaterialExpressionSubtract, -1300, 0)
    g.link(world_z, height, "A")
    g.link(ground_height, height, "B")
    height_ratio = g.node(unreal.MaterialExpressionDivide, -1150, 0)
    g.link(height, height_ratio, "A")
    g.link(dirt_height, height_ratio, "B")
    fade = g.node(unreal.MaterialExpressionSaturate, -1000, 0)
    g.link(height_ratio, fade)
    near_base = g.node(unreal.MaterialExpressionOneMinus, -880, 0)
    g.link(fade, near_base)
    dirt_mask = g.node(unreal.MaterialExpressionMultiply, -760, 0)
    g.link(near_base, dirt_mask, "A")
    g.link(dirt_amount, dirt_mask, "B")

    # Low-frequency tonal breakup in world space.
    macro_position = g.node(unreal.MaterialExpressionDivide, -1450, 520)
    g.link(world_position, macro_position, "A")
    g.link(macro_scale, macro_position, "B")
    noise = g.node(unreal.MaterialExpressionNoise, -1300, 520)
    g.optional(noise, "scale", 1.0)
    g.optional(noise, "levels", 2)
    g.optional(noise, "output_min", 0.0)
    g.optional(noise, "output_max", 1.0)
    g.link(macro_position, noise, "Position")
    noise_alpha = g.node(unreal.MaterialExpressionSaturate, -1150, 520)
    g.link(noise, noise_alpha)
    low = g.node(unreal.MaterialExpressionSubtract, -1300, 700, const_a=1.0)
    g.link(macro_variation, low, "B")
    high = g.node(unreal.MaterialExpressionAdd, -1300, 820, const_a=1.0)
    g.link(macro_variation, high, "B")
    breakup = g.node(unreal.MaterialExpressionLinearInterpolate, -1000, 600)
    g.link(low, breakup, "A")
    g.link(high, breakup, "B")
    g.link(noise_alpha, breakup, "Alpha")

    tinted = g.node(unreal.MaterialExpressionMultiply, -800, -460)
    g.link(base_tint, tinted, "A")
    g.link(breakup, tinted, "B")
    dirty = g.node(unreal.MaterialExpressionMultiply, -600, -360)
    g.link(tinted, dirty, "A")
    g.link(dirt_tint, dirty, "B")
    base_color = g.node(unreal.MaterialExpressionLinearInterpolate, -400, -420)
    g.link(tinted, base_color, "A")
    g.link(dirty, base_color, "B")
    g.link(dirt_mask, base_color, "Alpha")

    rough_dirt = g.node(unreal.MaterialExpressionMultiply, -560, 140, const_b=0.12)
    g.link(dirt_mask, rough_dirt, "A")
    rough_sum = g.node(unreal.MaterialExpressionAdd, -400, 220)
    g.link(roughness, rough_sum, "A")
    g.link(rough_dirt, rough_sum, "B")
    rough_out = g.node(unreal.MaterialExpressionSaturate, -260, 220)
    g.link(rough_sum, rough_out)

    g.output(base_color, unreal.MaterialProperty.MP_BASE_COLOR)
    g.output(rough_out, unreal.MaterialProperty.MP_ROUGHNESS)
    g.output(metallic, unreal.MaterialProperty.MP_METALLIC)

    # Detail pieces are drawn through InstancedStaticMeshComponents.
    g.optional(material, "used_with_instanced_static_meshes", True)


def validate(material):
    for material_property in (unreal.MaterialProperty.MP_BASE_COLOR,
                              unreal.MaterialProperty.MP_ROUGHNESS,
                              unreal.MaterialProperty.MP_METALLIC):
        try:
            if MEL.get_material_property_input_node(material, material_property) is None:
                fail("Entree non reliee: %s" % material_property)
        except AttributeError:
            # Older API without get_material_property_input_node: the connect calls already
            # reported success, nothing more to check.
            return


def main():
    material = get_or_create_master()
    try:
        build_graph(material)
        try:
            MEL.layout_material_expressions(material)
        except Exception:
            pass
        MEL.recompile_material(material)
        validate(material)
        if not EAL.save_loaded_asset(material, False):
            fail("Sauvegarde impossible: " + MASTER_PATH)
    except Exception:
        # Never leave a half-built master behind: the game would pick it up.
        try:
            EAL.delete_asset(MASTER_PATH)
            warn("Asset incomplet supprime: " + MASTER_PATH)
        except Exception:
            pass
        raise
    log("Master material pret: " + MASTER_PATH)


try:
    main()
except Exception:
    unreal.log_error("[UltraRealFPS Materials] " + traceback.format_exc())
    raise
