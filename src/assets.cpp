#include "assets.hpp"

#include "raymath.h"

#include <unordered_map>

static std::unordered_map<const Texture *, Material> _normalMaterials;
static std::unordered_map<const Texture *, Material> _instancedMaterials;
static std::unordered_map<std::string, Texture2D> _textures;
static Shader _mapShader;

static std::unordered_map<std::string, Model> _shapes;
static std::unordered_map<const Model *, RenderTexture2D> _shapeIcons;
static Camera _iconCamera;

static Font _font;

void Assets::Initialize() {
    //Initialize instanced shader for map geometry
    _mapShader = LoadShader("assets/shaders/map_geom.vs", "assets/shaders/map_geom.fs");
    _mapShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(_mapShader, "mvp");
    _mapShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(_mapShader, "viewPos");
    _mapShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(_mapShader, "instanceTransform");

    _font = LoadFont("assets/fonts/dejavu.fnt");

    _iconCamera = { 0 };
    _iconCamera.up = (Vector3){ 0.0f, -1.0f, 0.0f };
    _iconCamera.fovy = 45.0f;
    _iconCamera.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(_iconCamera, CAMERA_ORBITAL);
    _iconCamera.position = (Vector3){ 4.0f, 4.0f, 4.0f };
    _iconCamera.target = Vector3Zero();
}

void Assets::Update() {
    UpdateCamera(&_iconCamera);

    for (const auto& [model, target] : _shapeIcons) {
        DrawShapeIcon(target, model);
    }
}

void Assets::Unload() {
    for (const auto& [model, target] : _shapeIcons) {
        UnloadRenderTexture(target);
    }
}

Texture *Assets::GetTexture(const std::string texturePath) {
    if (_textures.find(texturePath) == _textures.end()) {
        _textures[texturePath] = LoadTexture(texturePath.c_str());
    }
    return &_textures[texturePath];
}

Material *Assets::GetMaterialForTexture(const std::string texturePath, bool instanced) {
    return GetMaterialForTexture(GetTexture(texturePath), instanced);
}

Material *Assets::GetMaterialForTexture(const Texture2D *texture, bool instanced) {
    if (instanced) {
        if (_instancedMaterials.find(texture) == _instancedMaterials.end()) {
            Material mat = LoadMaterialDefault();
            SetMaterialTexture(&mat, MATERIAL_MAP_ALBEDO, *texture);
            mat.shader = _mapShader;
            _instancedMaterials[texture] = mat; 
        }
        return &_instancedMaterials[texture];
    } else {
        if (_normalMaterials.find(texture) == _normalMaterials.end()) {
            Material mat = LoadMaterialDefault();
            SetMaterialTexture(&mat, MATERIAL_MAP_ALBEDO, *texture);
            _normalMaterials[texture] = mat; 
        }
        return &_normalMaterials[texture];
    }
}

Model *Assets::GetShape(const std::string modelPath) {
    if (_shapes.find(modelPath) == _shapes.end()) {
        _shapes[modelPath] = LoadModel(modelPath.c_str());
    }
    return &_shapes[modelPath];
}

Texture2D *Assets::GetShapeIcon(const Model *shape) {
    if (!shape) return nullptr;

    if (_shapeIcons.find(shape) == _shapeIcons.end()) {
        //Generate icon by rendering the shape onto a texture.
        RenderTexture2D target = LoadRenderTexture(SHAPE_ICON_SIZE, SHAPE_ICON_SIZE);

        DrawShapeIcon(target, shape);

        _shapeIcons[shape] = target;
    }
    return &_shapeIcons[shape].texture;
}

void Assets::DrawShapeIcon(const RenderTexture2D& target, const Model *shape) {
    BeginTextureMode(target);
    ClearBackground(BLACK);
    BeginMode3D(_iconCamera);

    DrawModelWiresEx(*shape, Vector3Zero(), (Vector3){0.0f, 1.0f, 0.0f}, GetTime() * 180.0f, Vector3One(), WHITE);

    EndMode3D();
    EndTextureMode();
}

Font *Assets::GetFont() {
    return &_font;
}