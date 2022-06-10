#include "pick_mode.hpp"

#include "extras/raygui.h"
#include "raymath.h"

#include <cstring>
#include <stack>

#include "assets.hpp"
#include "files_util.hpp"

const int FRAME_SIZE = 64;
const int FRAME_MARGIN = 16;
const int FRAME_SPACING = (FRAME_SIZE + FRAME_MARGIN * 2);

#define SEARCH_BUFFER_SIZE 256

PickMode::PickMode(AppContext *context, Mode mode)
    : _mode(mode),
      _context(context),
      _scroll(Vector2Zero()),
      _selectedFrame(nullptr),
      _searchFilterFocused(false)
{
    _searchFilterBuffer = (char *)calloc(SEARCH_BUFFER_SIZE, sizeof(char));
}

PickMode::~PickMode()
{
    free(_searchFilterBuffer);
}

void PickMode::_GetFrames(std::string rootDir)
{
    std::stack<std::string> dirs;
    dirs.push(rootDir);

    while (!dirs.empty())
    {
        std::string dir = dirs.top();
        dirs.pop();

        char **files = nullptr;
        int fileCount = 0;
        //This function will free the memory stored in `files` every time it is called, so be careful not to call it during iteration.
        files = GetDirectoryFiles(dir.c_str(), &fileCount);

        for (int f = 0; f < fileCount; ++f)
        {
            std::string fullPath = BuildPath({dir, files[f]});
            if (DirectoryExists(fullPath.c_str()) && strcmp(files[f], ".") != 0 && strcmp(files[f], "..") != 0)
            {
                dirs.push(fullPath);
            }
            else if (_mode == Mode::TEXTURES && IsFileExtension(files[f], ".png"))
            {
                Frame frame = {
                    .tex = Assets::GetTexture(fullPath),
                    .shape = nullptr,
                    .label = fullPath};
                _frames.push_back(frame);
            }
            else if (_mode == Mode::SHAPES && IsFileExtension(files[f], ".obj"))
            {
                Model *shape = Assets::GetShape(fullPath);
                Frame frame = {
                    .tex = Assets::GetShapeIcon(shape),
                    .shape = shape,
                    .label = fullPath};
                _frames.push_back(frame);
            }
        }
    }
}

void PickMode::OnEnter()
{
    _selectedFrame = nullptr;
    _frames.clear();

    if (_mode == Mode::TEXTURES)
        _GetFrames(_context->texturesDir);
    else if (_mode == Mode::SHAPES)
        _GetFrames(_context->shapesDir);

    ClearDirectoryFiles();
}

void PickMode::OnExit()
{
}

void PickMode::Update()
{
    if (_selectedFrame)
    {
        if (_mode == Mode::TEXTURES)
            _context->selectedTexture = _selectedFrame->tex;
        else if (_mode == Mode::SHAPES)
            _context->selectedShape = _selectedFrame->shape;
    }
    
    //Filter frames by the search text. If the search text is contained anywhere in the file path, then it passes.
    _filteredFrames.clear();
    for (const Frame& frame : _frames) {
        std::string lowerCaseLabel = TextToLower(frame.label.c_str());
        if (strlen(_searchFilterBuffer) == 0 || lowerCaseLabel.find(TextToLower(_searchFilterBuffer)) != std::string::npos)
        {
            _filteredFrames.push_back(frame);
        }
    }
}

void PickMode::Draw()
{
    Rectangle _framesView = (Rectangle){32, 96, (float)GetScreenWidth() - 64, (float)GetScreenHeight() - 128};
    const int FRAMES_PER_ROW = (int)_framesView.width / FRAME_SPACING;
    Rectangle _framesContent = (Rectangle){
        0, 0, 
        _framesView.width - 16, ceilf((float)_filteredFrames.size() / FRAMES_PER_ROW) * FRAME_SPACING + 64
    };

    GuiLabel((Rectangle){32, 32, 128, 32}, "SEARCH:");
    Rectangle searchBoxRect = (Rectangle){128, 32, (float)GetScreenWidth() / 3.0f, 32};
    if (GuiTextBox(searchBoxRect, _searchFilterBuffer, SEARCH_BUFFER_SIZE, _searchFilterFocused))
    {
        _searchFilterFocused = !_searchFilterFocused;
    }
    if (GuiButton((Rectangle){searchBoxRect.x + searchBoxRect.width + 4, searchBoxRect.y, 96, 32}, "Clear"))
    {
        memset(_searchFilterBuffer, 0, SEARCH_BUFFER_SIZE * sizeof(char));
    }

    Rectangle scissorRect = GuiScrollPanel(_framesView, NULL, _framesContent, &_scroll);

    // Drawing the scrolling view
    BeginScissorMode(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);
    {
        // Position frames based on order and search filter.
        int f = 0;
        for (Frame &frame : _filteredFrames)
        {
            Rectangle rect = (Rectangle){
                .x = _framesView.x + FRAME_MARGIN + (f % FRAMES_PER_ROW) * FRAME_SPACING + _scroll.x,
                .y = _framesView.y + FRAME_MARGIN + (f / FRAMES_PER_ROW) * FRAME_SPACING + _scroll.y,
                .width = FRAME_SIZE,
                .height = FRAME_SIZE};
                
            ++f;

            if (CheckCollisionPointRec(GetMousePosition(), scissorRect) && CheckCollisionPointRec(GetMousePosition(), rect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                _selectedFrame = &frame;
            }

            DrawRectangle(rect.x - 2, rect.y - 2, rect.width + 4, rect.height + 4, BLACK);
            DrawTextureQuad(*frame.tex, Vector2One(), Vector2Zero(), rect, WHITE);
            DrawRectangleLinesEx((Rectangle){rect.x - 2, rect.y - 2, rect.width + 4, rect.height + 4}, 2.0f, _selectedFrame == &frame ? WHITE : BLACK);
        }
    }
    EndScissorMode();

    // Draw the text showing the selected frame's label.
    if (_selectedFrame)
    {
        std::string selectString = std::string("Selected: ") + _selectedFrame->label;

        GuiLabel((Rectangle){
                     32, searchBoxRect.y + searchBoxRect.height + 4, (float)GetScreenWidth() / 2.0f, 16},
                 selectString.c_str());
    }
}