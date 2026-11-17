//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "game/dialog/dialog.h"

#include <cstdio>
#include <stack>

#include "debug/debug_log.h"
#include "device/sys_events.h"
#include "font/fonts.h"
#include "game/dialog/display.h"
#include "game/dialog/parser.h"
#include "game/dialog_def.h"
#include "game/dynobj/script_dialog_options_rendering.h"
#include "game/dynobj/script_drawing_surface.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/render/dynamic_sprite.h"
#include "game/render/overlay.h"
#include "game/script.h"
#include "game/system.h"
#include "game/translation.h"
#include "game/ui/gui.h"
#include "game/ui/mouse.h"
#include "game/world/character.h"
#include "game/world/properties.h"
#include "game/world/room.h"
#include "gfx/ddb.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/scriptexecutor.h"
#include "util/common.h"
#include "util/keycode.h"

using namespace AGS::Common;
using namespace AGS::Engine;
class DialogExec;
class DialogOptionsState;

std::unique_ptr<DialogExec> dialogExec;         // current running dialog state
std::unique_ptr<DialogOptionsState> dialogOpts; // current running dialog options


void RunDialogOption(int dialog_id, int option)
{
    if ((dialog_id < 0) || (dialog_id >= Game::Current().DialogCount))
        quit("!Dialog.Start: invalid topic number specified");

    can_run_delayed_command();

    if (handle_state_change_in_dialog_request("RunDialog", DIALOG_NEWTOPIC + dialog_id, option))
        return; // handled

    if (is_inside_script())
        get_executingscript()->QueueAction(PostScriptAction(ePSARunDialog, dialog_id, option, "RunDialog"));
    else
        do_conversation(dialog_id);
}

void RunDialog(int dialog_id)
{
    RunDialogOption(dialog_id, 0);
}

void StopDialog()
{
    // NOTE: dialog options may be displayed with Dialog.DisplayOptions() too
    if (!is_in_dialog() && !is_in_dialogoptions())
    {
        debug_script_log("StopDialog: not currently in dialog, nor dialog options are displayed, ignored");
        return;
    }

    if (handle_state_change_in_dialog_request("StopDialog", DIALOG_STOP))
        return; // handled

    if (is_inside_script() && get_can_run_delayed_command())
        get_executingscript()->QueueAction(PostScriptAction(ePSAStopDialog, 0, "StopDialog"));
    else
        schedule_dialog_stop();
}

void SetDialogOption(int dialog_id, int option, int state, bool dlg_script = false)
{
    Game &game = Game::Current();
    if ((dialog_id < 0) || (dialog_id >= game.DialogCount))
        quit("!SetDialogOption: Invalid topic number specified");
    if ((option < 1) || (option > game.Dialogs[dialog_id].GetOptionCount()))
    {
        // TODO: no caller passes dlg_script = true any more (pre-3.1.1 games had "dialog scripts" in a
        // different language, whose "option-on/off" commands were more permissive); this branch and the
        // parameter can go.
        if (dlg_script)
        {
            Debug::Printf(kDbgGroup_Game, kDbgMsg_Error, "SetDialogOption: Invalid option number specified (%d : %d)",
                          dialog_id, option);
            return;
        }
        quit("!SetDialogOption: Invalid option number specified");
    }
    option--;

    game.Dialogs[dialog_id].Options[option].Flags &= ~DFLG_ON;
    if ((state == 1) && ((game.Dialogs[dialog_id].Options[option].Flags & DFLG_OFFPERM) == 0))
        game.Dialogs[dialog_id].Options[option].Flags |= DFLG_ON;
    else if (state == 2)
        game.Dialogs[dialog_id].Options[option].Flags |= DFLG_OFFPERM;
}

int GetDialogOption(int dialog_id, int option)
{
    Game &game = Game::Current();
    if ((dialog_id < 0) || (dialog_id >= game.DialogCount))
        quit("!GetDialogOption: Invalid topic number specified");
    if ((option < 1) || (option > game.Dialogs[dialog_id].GetOptionCount()))
        quit("!GetDialogOption: Invalid option number specified");
    option--;

    if (game.Dialogs[dialog_id].Options[option].Flags & DFLG_OFFPERM)
        return 2;
    if (game.Dialogs[dialog_id].Options[option].Flags & DFLG_ON)
        return 1;
    return 0;
}

void Dialog_Start(ScriptDialog *script_dialog)
{
    RunDialog(script_dialog->id);
}

void Dialog_StartOption(ScriptDialog *script_dialog, int option)
{
    RunDialogOption(script_dialog->id, option);
}

// Returned by show_dialog_options() when the text parser input was used
#define CHOSE_TEXTPARSER (-3053)
// Values of Dialog.DisplayOptions' sayChosenOption: say the chosen option's text unless it has the
// no-repeat flag, always say it, never say it
#define SAYCHOSEN_USEFLAG 1
#define SAYCHOSEN_YES     2
#define SAYCHOSEN_NO      3

int Dialog_DisplayOptions(ScriptDialog *script_dialog, int say_chosen_option)
{
    if ((say_chosen_option < 1) || (say_chosen_option > 3))
        quit("!Dialog.DisplayOptions: invalid parameter passed");

    int chosen = show_dialog_options(script_dialog->id, (Game::Current().Options[OPT_RUNGAMEDLGOPTS] != 0));

    if (chosen > 0)
    {
        run_dialog_option(script_dialog->id, chosen, say_chosen_option, false /* don't run script */);
    }

    if (chosen != CHOSE_TEXTPARSER)
    {
        chosen++;
    }
    return chosen;
}

void Dialog_SetOptionState(ScriptDialog *script_dialog, int option, int new_state)
{
    SetDialogOption(script_dialog->id, option, new_state);
}

int Dialog_GetOptionState(ScriptDialog *script_dialog, int option)
{
    return GetDialogOption(script_dialog->id, option);
}

int Dialog_HasOptionBeenChosen(ScriptDialog *script_dialog, int option)
{
    Game &game = Game::Current();
    if ((option < 1) || (static_cast<uint32_t>(option) > game.Dialogs[script_dialog->id].GetOptionCount()))
        quit("!Dialog.HasOptionBeenChosen: Invalid option number specified");
    option--; // option id is 1-based in script, and 0 is entry point

    if (game.Dialogs[script_dialog->id].Options[option].Flags & DFLG_HASBEENCHOSEN)
        return 1;
    return 0;
}

void Dialog_SetHasOptionBeenChosen(ScriptDialog *script_dialog, int option, bool chosen)
{
    Game &game = Game::Current();
    if (option < 1 || static_cast<uint32_t>(option) > game.Dialogs[script_dialog->id].GetOptionCount())
    {
        quit("!Dialog.HasOptionBeenChosen: Invalid option number specified");
    }
    option--; // option id is 1-based in script, and 0 is entry point
    if (chosen)
    {
        game.Dialogs[script_dialog->id].Options[option].Flags |= DFLG_HASBEENCHOSEN;
    }
    else
    {
        game.Dialogs[script_dialog->id].Options[option].Flags &= ~DFLG_HASBEENCHOSEN;
    }
}

int Dialog_GetOptionCount(ScriptDialog *script_dialog)
{
    return Game::Current().Dialogs[script_dialog->id].GetOptionCount();
}

int Dialog_GetShowTextParser(ScriptDialog *script_dialog)
{
    return (Game::Current().Dialogs[script_dialog->id].Flags & DTFLG_SHOWPARSER) ? 1 : 0;
}

const char *Dialog_GetOptionText(ScriptDialog *script_dialog, int option)
{
    Game &game = Game::Current();
    if ((option < 1) || (static_cast<uint32_t>(option) > game.Dialogs[script_dialog->id].GetOptionCount()))
        quit("!Dialog.GetOptionText: Invalid option number specified");

    option--; // option id is 1-based in script, and 0 is entry point

    return CreateNewScriptString(game.Dialogs[script_dialog->id].Options[option].Text);
}

void Dialog_SetOptionText(ScriptDialog *script_dialog, int option, const char *text)
{
    Game &game = Game::Current();
    if ((option < 1) || (static_cast<uint32_t>(option) > game.Dialogs[script_dialog->id].GetOptionCount()))
        quit("!Dialog.GetOptionText: Invalid option number specified");

    option--; // option id is 1-based in script, and 0 is entry point

    auto &dialog_option = game.Dialogs[script_dialog->id].Options[option];
    dialog_option.Text = text;
    dialog_option.Flags =
        (dialog_option.Flags & ~DFLG_TEXTSET) | (DFLG_TEXTSET * (dialog_option.OriginalText != dialog_option.Text));
}

int Dialog_GetID(ScriptDialog *script_dialog)
{
    return script_dialog->id;
}

const char *Dialog_GetScriptName(ScriptDialog *script_dialog)
{
    return CreateNewScriptString(Game::Current().Dialogs[script_dialog->id].ScriptName);
}

int DialogOptions_GetAreDisplayed()
{
    return is_in_dialogoptions();
}

int DialogOptions_GetBulletGraphic()
{
    return Game::Current().DialogBulletSprite;
}

void DialogOptions_SetBulletGraphic(int sprite)
{
    Game::Current().DialogBulletSprite = sprite;
}

int DialogOptions_GetDrawMode()
{
    return Game::Current().Options[OPT_DIALOGOPTIONSDRAWMODE];
}

void DialogOptions_SetDrawMode(int draw_mode)
{
    Game::Current().Options[OPT_DIALOGOPTIONSDRAWMODE] = draw_mode;
}

int DialogOptions_GetFont()
{
    return Game::Current().Play.dialog_options_font;
}

void DialogOptions_SetFont(int font_id)
{
    font_id = ValidateFontNumber("Dialog.SetOptionsFont", font_id);
    Game::Current().Play.dialog_options_font = font_id;
}

int DialogOptions_GetItemNumbering()
{
    return Game::Current().Options[OPT_DIALOGNUMBERED];
}

void DialogOptions_SetItemNumbering(int style)
{
    Game::Current().Options[OPT_DIALOGNUMBERED] = style;
}

int DialogOptions_GetHighlightColor()
{
    return Game::Current().Play.dialog_options_highlight_color;
}

void DialogOptions_SetHighlightColor(int color)
{
    Game::Current().Play.dialog_options_highlight_color = color;
}

int DialogOptions_GetReadColor()
{
    return Game::Current().Play.read_dialog_option_colour;
}

void DialogOptions_SetReadColor(int color)
{
    Game::Current().Play.read_dialog_option_colour = color;
}

int DialogOptions_GetTextAlignment()
{
    return Game::Current().Play.dialog_options_textalign;
}

void DialogOptions_SetTextAlignment(int align)
{
    Game::Current().Play.dialog_options_textalign = (HorAlignment)align;
}

int DialogOptions_GetZOrder()
{
    return Game::Current().Play.dialog_options_zorder;
}

void DialogOptions_SetZOrder(int zorder)
{
    Game::Current().Play.dialog_options_zorder = zorder;
}

int DialogOptions_GetItemGap()
{
    return Game::Current().Options[OPT_DIALOGGAP];
}

void DialogOptions_SetItemGap(int gap)
{
    Game::Current().Options[OPT_DIALOGGAP] = gap;
}

ScriptGUI *DialogOptions_GetTemplateGUI()
{
    Game &game = Game::Current();
    // NOTE: historically 0 meant "no gui", so gui id 0 cannot be used here
    if (game.Options[OPT_DIALOGIFACE] <= 0 || game.Options[OPT_DIALOGIFACE] >= game.GuiCount)
        return nullptr;
    return &game.ScGuis[game.Options[OPT_DIALOGIFACE]];
}

void DialogOptions_SetTemplateGUI(ScriptGUI *sgui)
{
    if (sgui->id == 0)
    {
        debug_script_warn("Dialog.SetGUI: cannot assign GUI with ID 0 to dialog options");
        return;
    }

    Game::Current().Options[OPT_DIALOGIFACE] = sgui->id;
}

int DialogOptions_GetGUIX()
{
    return Game::Current().Play.dialog_options_gui_x;
}

void DialogOptions_SetGUIX(int x)
{
    Game::Current().Play.dialog_options_gui_x = x;
}

int DialogOptions_GetGUIY()
{
    return Game::Current().Play.dialog_options_gui_y;
}

void DialogOptions_SetGUIY(int y)
{
    Game::Current().Play.dialog_options_gui_y = y;
}

int DialogOptions_GetPaddingX()
{
    return Game::Current().Play.dialog_options_pad_x;
}

void DialogOptions_SetPaddingX(int x)
{
    Game::Current().Play.dialog_options_pad_x = x;
}

int DialogOptions_GetPaddingY()
{
    return Game::Current().Play.dialog_options_pad_y;
}

void DialogOptions_SetPaddingY(int y)
{
    Game::Current().Play.dialog_options_pad_y = y;
}

int DialogOptions_GetMaxGUIWidth()
{
    return Game::Current().Play.max_dialogoption_width;
}

void DialogOptions_SetMaxGUIWidth(int width)
{
    Game::Current().Play.max_dialogoption_width = width;
}

int DialogOptions_GetMinGUIWidth()
{
    return Game::Current().Play.min_dialogoption_width;
}

void DialogOptions_SetMinGUIWidth(int width)
{
    Game::Current().Play.min_dialogoption_width = width;
}

int Dialog_GetProperty(ScriptDialog *script_dialog, const char *property)
{
    Game &game = Game::Current();
    return get_int_property(game.dialogProps[script_dialog->id], game.Play.dialogProps[script_dialog->id], property);
}

const char *Dialog_GetTextProperty(ScriptDialog *script_dialog, const char *property)
{
    Game &game = Game::Current();
    return get_text_property_dynamic_string(game.dialogProps[script_dialog->id],
                                            game.Play.dialogProps[script_dialog->id], property);
}

bool Dialog_SetProperty(ScriptDialog *script_dialog, const char *property, int value)
{
    return set_int_property(Game::Current().Play.dialogProps[script_dialog->id], property, value);
}

bool Dialog_SetTextProperty(ScriptDialog *script_dialog, const char *property, const char *value)
{
    return set_text_property(Game::Current().Play.dialogProps[script_dialog->id], property, value);
}

//=============================================================================
// dialog manager stuff

// Results of running a dialog entry or option: show the same options again, end the dialog, or return to
// the previous topic; a value >= 0 is the number of a topic to go to
#define RUN_DIALOG_STAY          -1
#define RUN_DIALOG_STOP_DIALOG   -2
#define RUN_DIALOG_GOTO_PREVIOUS -4

// Runs the game script's dialog_request(param) callback (called when text parser input was committed);
// returns a RUN_DIALOG_* code, or the number of the topic the callback asked to go to
static int run_dialog_request(int param)
{
    Game &game = Game::Current();
    game.Play.stop_dialog_at_end = DIALOG_RUNNING;
    RuntimeScriptValue params[]{param};
    RunScriptFunction(game.Scripts.Global.get(), "dialog_request", 1, params);

    if (game.Play.stop_dialog_at_end == DIALOG_STOP)
    {
        game.Play.stop_dialog_at_end = DIALOG_NONE;
        return RUN_DIALOG_STOP_DIALOG;
    }
    if (game.Play.stop_dialog_at_end >= DIALOG_NEWTOPIC)
    {
        int topic_id = game.Play.stop_dialog_at_end - DIALOG_NEWTOPIC;
        game.Play.stop_dialog_at_end = DIALOG_NONE;
        return topic_id;
    }
    if (game.Play.stop_dialog_at_end >= DIALOG_NEWROOM)
    {
        int room_id = game.Play.stop_dialog_at_end - DIALOG_NEWROOM;
        game.Play.stop_dialog_at_end = DIALOG_NONE;
        NewRoom(room_id);
        return RUN_DIALOG_STOP_DIALOG;
    }
    game.Play.stop_dialog_at_end = DIALOG_NONE;
    return RUN_DIALOG_STAY;
}

// Runs the dialog script function of the given dialog for an option (0 is the starting entry) and returns
// a RUN_DIALOG_* code or the topic number to go to; entry_point is not used, the function is found by
// dialog_id
int run_dialog_script(int dialog_id, int /*entry_point*/, int option_index)
{
    Game &game = Game::Current();
    game.Speech.SaidSpeechLine = 0;
    int result = RUN_DIALOG_STAY;

    if (game.Scripts.Dialog)
    {
        char func_name[100];
        snprintf(func_name, sizeof(func_name), "_run_dialog%d", dialog_id);
        RuntimeScriptValue params[]{option_index};
        RunScriptFunction(game.Scripts.Dialog.get(), func_name, 1, params);
        result = scriptExecutor->GetReturnValue();
    }

    // Changed room while running the dialog: close the dialog immediately
    if (game.RoomChange.InNewRoom != kEnterRoom_None)
    {
        schedule_dialog_stop();
        return RUN_DIALOG_STOP_DIALOG;
    }

    if (game.Speech.SaidSpeechLine > 0)
    {
        // the line below fixes the problem with the close-up face remaining on the
        // screen after they finish talking; however, it makes the dialog options
        // area flicker when going between topics.
        // TODO: find a better solution rather than disable/enable iface?
        // this causes all gui texture reupdate twice, and other things...
        DisableInterface();
        UpdateGameOnce(); // redraw the screen to make sure it looks right
        EnableInterface();
        // if we're not about to abort the dialog, switch back to arrow
        if (result != RUN_DIALOG_STOP_DIALOG)
            set_cursor_look(game.GetModePointer());
    }

    return result;
}

// A displayed dialog option: OptionID is its index in the topic's options (0-based), Position is its
// rectangle in the options area, set when the options are drawn (empty before that)
// TODO: make this a member of DialogOptionsState class
struct DialogOptionItem
{
    int OptionID = -1;
    Rect Position;

    DialogOptionItem() = default;
    DialogOptionItem(int option_id) : OptionID(option_id) {}
};

// Draws the displayed options (items) onto dst, starting at at_x, at_y and wrapped to area_width, and stores
// each item's Position; mouse_over_item is the highlighted one. Returns the Y coordinate where the next
// line would start, below the last option.
// TODO: make this a member of DialogOptionsState class
// TODO: don't use global variables inside this function
// TODO: gather parameters into struct(s)
// TODO: limit by area height too
static int write_dialog_options(Bitmap *dst, int at_x, int at_y, int area_width, int bullet_width, int bullet_sprite,
                                int bullet_sprite_width, int font_id, int line_spacing, int selected_color,
                                const Dialog *topic, int num_displayed, int mouse_over_item,
                                std::vector<DialogOptionItem> &items)
{
    Game &game = Game::Current();
    // Left-to-right text direction flag
    const bool ltr_position = (game.Options[OPT_RIGHTLEFTWRITE] == 0);

    // Configure positioning settings
    const HorAlignment text_align = game.Play.dialog_options_textalign;
    const std::pair<int, int> wrap_range = std::make_pair(at_x + bullet_width, at_x + area_width - 1);
    // Extra offset for 2nd, 3rd etc lines of the same option, *relative* to the first line
    // TODO: make this value dynamic, based on sizes etc?
    const int min_multiline_off = 9;
    int first_line_off = 0, multiline_off = 0;
    switch (text_align)
    {
    case kHAlignRight:
        if (ltr_position)
        {
            // don't offset at all when right-aligned (does not look good)
            first_line_off = 0;
            multiline_off = 0;
        }
        else
        {
            // when RTL is right aligned: apply reverse offset;
            // next lines are extra offset by either bullet_wid or min offset
            first_line_off = -bullet_width;
            multiline_off = -bullet_width - std::max(0, min_multiline_off - bullet_width);
        }
        break;
    case kHAlignCenter:
        // when centering we negate wrapping range offset by half, so truly centering;
        // next lines are still offset by either bullet_wid or min offset
        first_line_off = -bullet_width / 2;
        multiline_off = -bullet_width / 2 + std::max(0, min_multiline_off - bullet_width);
        break;
    default:
        if (ltr_position)
        {
            // when left aligned, next lines are offset by either bullet_wid or min offset
            first_line_off = 0;
            multiline_off = std::max(0, min_multiline_off - bullet_width);
        }
        else
        {
            // left align for RTL is like right align for LTR, except we need to inverse bullet_wid
            first_line_off = -bullet_width;
            multiline_off = -bullet_width;
        }
        break;
    }

    int cur_y = at_y; // next line's Y position
    for (int item_index = 0; item_index < num_displayed; ++item_index)
    {
        color_t text_color = 0;
        if ((topic->Options[items[item_index].OptionID].Flags & DFLG_HASBEENCHOSEN) &&
            (game.Play.read_dialog_option_colour >= 0))
        {
            // 'read' colour
            text_color = dst->GetCompatibleColor(game.Play.read_dialog_option_colour);
        }
        else
        {
            // 'unread' colour
            text_color = dst->GetCompatibleColor(game.PlayerChar->SpeechColor);
        }

        if (mouse_over_item == item_index)
        {
            // If the normal colour is the same as highlight col, then fallback to another
            // TODO: don't use hardcoded color 13 as a fallback; maybe don't do this fallback at all?
            if (text_color == dst->GetCompatibleColor(selected_color))
                text_color = GUI::GetStandardColorForBitmap(13);
            else
                text_color = dst->GetCompatibleColor(selected_color);
        }

        const char *draw_text =
            skip_voiceover_token(get_translation(topic->Options[items[item_index].OptionID].Text.GetCStr()));
        // TODO: make the line-splitting container also save line widths!
        break_up_text_into_lines(draw_text, Lines, wrap_range.second - wrap_range.first + 1, font_id);
        const int first_line_width = get_text_width_outlined(Lines[0].GetCStr(), font_id);
        const int first_line_at =
            AlignInHRange(wrap_range.first, wrap_range.second, 0, first_line_width, (FrameAlignment)text_align) +
            first_line_off;

        items[item_index].Position = RectWH(wrap_range.first, cur_y, wrap_range.second - wrap_range.first + 1,
                                            get_text_lines_height(font_id, Lines.Count()));
        if (bullet_sprite > 0)
        {
            if (ltr_position)
                draw_gui_sprite(dst, bullet_sprite, first_line_at - bullet_width, cur_y);
            else
                draw_gui_sprite(dst, bullet_sprite,
                                first_line_at + first_line_width + (bullet_width - bullet_sprite_width), cur_y);
        }
        if (game.Options[OPT_DIALOGNUMBERED] == kDlgOptNumbering)
        {
            if (ltr_position)
            {
                String number = String::FromFormat("%d. ", item_index + 1);
                wouttext_outline(dst, first_line_at - bullet_width + bullet_sprite_width, cur_y, font_id, text_color,
                                 number.GetCStr());
            }
            else
            {
                String number = String::FromFormat(" .%d", item_index + 1);
                wouttext_outline(dst, first_line_at + first_line_width, cur_y, font_id, text_color, number.GetCStr());
            }
        }

        for (size_t line_index = 0; line_index < Lines.Count(); ++line_index)
        {
            const int line_width =
                (line_index == 0) ? first_line_width : get_text_width_outlined(Lines[line_index].GetCStr(), font_id);
            const int line_at =
                AlignInHRange(wrap_range.first, wrap_range.second, 0, line_width, (FrameAlignment)text_align) +
                ((line_index == 0) ? first_line_off : multiline_off);
            wouttext_outline(dst, line_at, cur_y, font_id, text_color, Lines[line_index].GetCStr());
            cur_y += line_spacing;
        }

        if (item_index < num_displayed - 1)
            cur_y += game.Options[OPT_DIALOGGAP];
    }
    return cur_y;
}

// Draws the background color and image of the template GUI onto dst at x, y (its controls are not drawn)
void draw_gui_for_dialog_options(Bitmap *dst, GUIMain *gui, int x, int y)
{
    if (gui->GetBgColor() != 0)
    {
        color_t draw_color = dst->GetCompatibleColor(gui->GetBgColor());
        dst->FillRect(Rect(x, y, x + gui->GetWidth(), y + gui->GetHeight()), draw_color);
    }
    if (gui->GetBgImage() > 0)
        GfxUtil::DrawSpriteWithTransparency(dst, EngineServices::Current().Sprites[gui->GetBgImage()], x, y);
}

// Runs the script's dialog_options_get_dimensions for the dialog; returns true if it gave a non-empty area,
// which means the custom options rendering is used
bool get_custom_dialog_options_dimensions(ScriptDialogOptionsRendering *opt_render, int dialog_id)
{
    Game &game = Game::Current();
    opt_render->Reset();
    opt_render->dialogID = dialog_id;

    game.Scripts.GetDialogOptionsDimensions.Params[0].SetScriptObject(opt_render, opt_render);
    run_function_on_non_blocking_thread(&game.Scripts.GetDialogOptionsDimensions);
    return ((opt_render->width > 0) && (opt_render->height > 0));
}

// A special value of the active (hovered or chosen) option: the parser text box
#define DLG_OPTION_PARSER 99

// Dialog options state
class DialogOptionsState : public GameState
{
public:
    DialogOptionsState(Dialog *topic, int dialog_id, bool run_game_loops_in_background);
    ~DialogOptionsState() override;

    // Shows and run the loop until it's over
    void Show();
    // Request dialog options to stop;
    // Note that the stopping is scheduled and is performed as soon as
    // dialog options state receives control.
    void Stop();

    // Begin the state, initialize and prepare any resources
    void Begin() override;
    // End the state, release all resources
    void End() override;
    // Draw the state
    void Draw() override;
    // Update the state during a game tick
    bool Run() override;

    Dialog *GetDialog() const { return _dtop; }
    int GetChosenOption() const { return chose; }
    int GetOverlayHandle() const { return options_overlay_schandle; }

    // Tells if the dialog options are drawn using custom rendering (scripted)
    bool HasCustomRender() const { return usingCustomRendering; }
    // Returns list of displayed options.
    // NOTE: this only has a meaning for the standard options render, not custom render
    const std::vector<DialogOptionItem> &GetItems() const { return items; }

private:
    int CalcOptionsHeight(int padding);
    void CreateOverlay();
    void OnOverlayRemoved(ScreenOverlay &overlay);
    // Process all the buffered input events; returns if handled
    bool RunControls();
    // Process single key event; returns if handled
    bool RunKey(const KeyInput &key_input);
    // Process single mouse event; returns if handled
    bool RunMouse(eAGSMouseButton button, int x, int y);
    // Process mouse wheel scroll
    bool RunMouseWheel(int wheel);


    Dialog *const _dtop;
    const int dlgnum;
    const bool runGameLoopsInBackground;

    // dialog options rectangle on screen
    Rect position;
    // inner position of the options texts, relative to the gui
    Point inner_position;
    int usingfont;
    int lineheight;
    int linespacing;
    // the cursor look before the options opened; restored in End()
    int curswas;
    int bullet_wid;    // full width of bullet sprite + numbering
    int bullet_picwid; // bullet sprite width
    int number_wid;    // width of number component
    int needheight;    // height enough to accomodate dialog options texts
    // Backwards compatibility parameters (nasty stuff);
    // remove these whenever you don't care about keeping precise alignment in old games
    int line_x_off = 0; // extra X offset for option lines (normal GUI or def surf)
    int normal_area_counter_align_pad =
        0;                     // extra padding for the side *opposite* to text alignment (normal GUI or def surf)
    int tw_area_width_mod = 0; // inc/dec for the options drawable area (TextWindow case)
    int fixed_padding = 0;     // a fixed padding value added to the setup property (normal GUI or def surf)
    int linewrap_padding = 0;  // padding sum used only for linewrapping

    std::unique_ptr<GUITextBox> parserInput;
    int options_overlay_id = -1;
    int options_overlay_schandle = 0;

    // Text window essentials
    std::unique_ptr<Bitmap> text_window;
    Point text_window_offset;

    // List of displayed options and their precalculated states;
    // NOTE: this is only used in standard options render, not custom render
    std::vector<DialogOptionItem> items;

    // number of displayed options
    int numdisp;
    // chosen option: an index into the topic's options, DLG_OPTION_PARSER when the parser was used (End() then
    // turns it into CHOSE_TEXTPARSER), -1 while nothing is chosen
    int chose;
    bool doStop = false;

    int parserActivated;

    int curyp; // current (latest) draw position of a option text
    bool needRedraw;
    bool is_textwindow;
    bool is_normalgui;
    bool usingCustomRendering;
    bool newCustomRender; // using the newer custom render API (OPT_DIALOGOPTIONSAPI >= 0, not the pre-3.4.0 one)
    bool redrawOnMouseMove;
    // width of a region within the gui where options are arranged;
    // includes internal gui padding (from both sides)
    int areawid;
    int forecol;

    int mouseison;

    ScriptDialogOptionsRendering *ccDialogOptionsRendering = nullptr;
};

DialogOptionsState::DialogOptionsState(Dialog *topic, int dialog_id, bool run_game_loops_in_background)
    : _dtop(topic),
      dlgnum(dialog_id),
      runGameLoopsInBackground(run_game_loops_in_background)
{
}

DialogOptionsState::~DialogOptionsState()
{
    if (options_overlay_id > 0)
    {
        remove_screen_overlay(options_overlay_id);
    }
    parserInput.reset();
}

int DialogOptionsState::CalcOptionsHeight(int padding)
{
    Game &game = Game::Current();
    // According to the options drawing logic:
    // * the distance between LINES in a multiline option is font linespacing;
    // * the distance between OPTIONS is font linespacing + OPT_DIALOGGAP;
    // * IF there's a parser input, we also add another linespacing + OPT_DIALOGGAP
    //   after the last option;
    // * IF there's no parser, then we add *font's graphical height* + OPT_DIALOGGAP,
    //   to ensure that the text is not going to be cut off visually.
    int need_height = 0;
    int total_lines = 0;

    // TODO: cache breaking text into lines, don't repeat the process in Draw
    for (int i = 0; i < numdisp; ++i)
    {
        const char *draw_text = skip_voiceover_token(get_translation(_dtop->Options[items[i].OptionID].Text.GetCStr()));
        break_up_text_into_lines(draw_text, Lines, areawid - (2 * padding + 2 + bullet_wid), usingfont);
        total_lines += Lines.Count();
    }
    need_height = linespacing * (total_lines - 1) + game.Options[OPT_DIALOGGAP] * (numdisp - 1);
    if (parserInput)
    {
        need_height += linespacing + game.Options[OPT_DIALOGGAP];
        need_height += parserInput->GetHeight() + game.Options[OPT_DIALOGGAP];
    }
    else
    {
        need_height += get_text_lines_height(usingfont, 1) + game.Options[OPT_DIALOGGAP];
    }
    return need_height;
}

void DialogOptionsState::CreateOverlay()
{
    Game &game = Game::Current();
    std::unique_ptr<Bitmap> opt_bitmap(
        new Bitmap(std::max(1, position.GetWidth()), std::max(1, position.GetHeight()), game.GetColorDepth()));
    options_overlay_id =
        add_screen_overlay(false /* screen */, position.Left, position.Top, OVER_CUSTOM, std::move(opt_bitmap));
    auto *overlay = get_overlay(options_overlay_id);
    if (overlay)
    {
        overlay->SetRemoveCallback([this](ScreenOverlay &overlay) { this->OnOverlayRemoved(overlay); });
        options_overlay_schandle = ccAssignObjectHandle(overlay->CreateScriptObject());

        // Set initial position and sorting order
        overlay->SetFixedPosition(position.Left, position.Top);
        overlay->SetZOrder(game.Play.dialog_options_zorder);
    }
}

void DialogOptionsState::OnOverlayRemoved(ScreenOverlay & /*overlay*/)
{
    options_overlay_schandle = ccRemoveObjectHandle(options_overlay_schandle);
    options_overlay_id = -1;
}

void DialogOptionsState::Show()
{
    Begin();
    Draw(); // prepare the options texture prior to the first Run()
    while (Run())
        ;
    End();
}

void DialogOptionsState::Stop()
{
    doStop = true;
}

void DialogOptionsState::Begin()
{
    Game &game = Game::Current();
    items.clear();

    doStop = false;
    chose = -1;
    // First of all, decide which options should be displayed this turn
    numdisp = 0;
    for (uint32_t i = 0; i < _dtop->GetOptionCount(); ++i)
    {
        if ((_dtop->Options[i].Flags & DFLG_ON) == 0)
            continue; // option is off

        if (_dtop->Options[i].Text.IsEmpty())
            continue; // do not add an empty option name into the display list

        // Add this option into the display list
        items.push_back(DialogOptionItem(i));
        numdisp++;
    }

    usingfont =
        (game.Play.dialog_options_font == FONT_UNDEFINED) ? game.Play.normal_font : game.Play.dialog_options_font;

    lineheight = get_font_height_outlined(usingfont);
    linespacing = get_font_linespacing(usingfont);
    curswas = game.Cursor.Look;
    bullet_wid = 0;
    bullet_picwid = 0;
    number_wid = 0;
    parserInput = nullptr;
    game.Speech.SaidText = 0;

    // TODO: the option arrangement logic here is a tremendous mess.
    // need to strictly define the terms and rules of this arrangement,
    // with defined text area and general padding, and any additional shifts,
    // and make sure that it's consistent.
    tw_area_width_mod = +1; // a fix vs split_lines mistake

    if (game.DialogBulletSprite > 0)
    {
        bullet_picwid = game.SpriteInfos[game.DialogBulletSprite].Width + 3;
    }

    // numbered options, leave space for the numbers
    bullet_wid = bullet_picwid;
    if (game.Options[OPT_DIALOGNUMBERED] == kDlgOptNumbering)
    {
        number_wid = get_text_width_outlined("9. ", usingfont);
        bullet_wid += number_wid;
    }

    game.Play.in_conversation++;
    set_cursor_look(game.GetModePointer());

    parserActivated = 0;
    if ((_dtop->Flags & DTFLG_SHOWPARSER) && (game.Play.disable_dialog_parser == 0))
    {
        parserInput.reset(new GUITextBox());
        parserInput->SetHeight(lineheight + 4);
        parserInput->SetShowBorder(true);
        parserInput->SetFont(usingfont);
    }

    is_normalgui = false;
    is_textwindow = false;
    position = {};
    inner_position = {};
    forecol = game.Play.dialog_options_highlight_color;

    mouseison = -1;
    usingCustomRendering = false;

    // Create script object of DialogOptionsRendering
    ccDialogOptionsRendering = new ScriptDialogOptionsRendering();
    ccAddObjectReference(ccRegisterManagedObject(ccDialogOptionsRendering, ccDialogOptionsRendering));

    if (((game.Options[OPT_DIALOGOPTIONSDRAWMODE] == kDialogOptDraw_Auto) ||
         (game.Options[OPT_DIALOGOPTIONSDRAWMODE] == kDialogOptDraw_Custom)) &&
        get_custom_dialog_options_dimensions(ccDialogOptionsRendering, dlgnum))
    {
        // Custom dialog options rendering
        usingCustomRendering = true;
        position = RectWH(ccDialogOptionsRendering->x, ccDialogOptionsRendering->y, ccDialogOptionsRendering->width,
                          ccDialogOptionsRendering->height);

        // Create script object of the related DrawingSurface.
        // NOTE: although DrawingSurface is only allowed to be used within "draw" callback,
        // we create it here, and keep until the dialog options close. This is done because,
        // despite a clear warning in the manual, users kept accessing it other callbacks
        // too in old games, and it worked because engine's implementation did not match
        // its own specification (so by accident).
        ScriptDrawingSurface *dopts_surface = new ScriptDrawingSurface();
        ccAddObjectReference(ccRegisterManagedObject(dopts_surface, dopts_surface));
        ccDialogOptionsRendering->surfaceToRenderTo = dopts_surface;
    }
    else if (game.Options[OPT_DIALOGIFACE] > 0)
    {
        // Use GUI or TextWindow GUI
        GUIMain *gui = &game.Guis[game.Options[OPT_DIALOGIFACE]];
        if (gui->IsTextWindow())
        {
            // Text-window, so do the QFG4-style speech options
            is_textwindow = true;
            forecol = gui->GetFgColor();
            const int padding = game.Guis[game.Options[OPT_DIALOGIFACE]].GetPadding();
            // Find the widest options line that will be in the text window,
            // and adjust the area width to that line's width, while clamping to min,max range
            const int max_width = game.Play.max_dialogoption_width;
            int max_line_width = 0;
            for (int i = 0; i < numdisp; ++i)
            {
                const char *draw_text =
                    skip_voiceover_token(get_translation(_dtop->Options[items[i].OptionID].Text.GetCStr()));
                break_up_text_into_lines(draw_text, Lines, max_width - ((2 * padding + 2) + bullet_wid), usingfont);
                max_line_width = std::max(max_line_width, game.TextLayout.LongestLine);
            }
            areawid = max_line_width + ((2 * padding + 2 + tw_area_width_mod) + bullet_wid);
            areawid = Math::Clamp(areawid, game.Play.min_dialogoption_width, game.Play.max_dialogoption_width);
            needheight = CalcOptionsHeight(padding);

            const int saved_width = areawid;
            const Rect &ui_view = game.Viewports.GetUIViewport();
            int text_off_x = 0, text_off_y = 0, win_y = ui_view.GetHeight() / 2 - (2 * padding + needheight) / 2;
            int win_x = ui_view.GetWidth() / 2 - areawid / 2;
            // shift window to the right if QG4-style full-screen pic
            if ((game.Options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4) && (game.Speech.SaidText > 0))
                win_x = (ui_view.GetWidth() - areawid) - 10;

            // needs to draw the right text window, not the default
            text_window = draw_text_window(&text_off_x, &text_off_y, &win_x, &win_y, &areawid, nullptr, needheight,
                                           game.Options[OPT_DIALOGIFACE], DisplayVars());
            // since draw_text_window increases the width, restore the inner placement
            areawid = saved_width;
            position = RectWH(win_x, win_y, text_window->GetWidth(), text_window->GetHeight());
            text_window_offset = Point(text_off_x, text_off_y);
        }
        else
        {
            // Normal GUI
            is_normalgui = true;
            position = gui->GetRect();

            areawid = gui->GetWidth();
            linewrap_padding = game.Play.dialog_options_pad_x + fixed_padding;
            needheight = CalcOptionsHeight(linewrap_padding);

            if (game.Options[OPT_DIALOGUPWARDS])
            {
                // They want the options upwards from the bottom
                // FIXME: this setting is lying: it does not reverse the order, only aligns opts to the bottom of GUI
                position.MoveToY((gui->GetY() + gui->GetHeight()) - needheight);
            }
        }
    }
    else
    {
        // Default plain surface
        const Rect &ui_view = game.Viewports.GetUIViewport();
        areawid = ui_view.GetWidth();
        linewrap_padding = game.Play.dialog_options_pad_x + fixed_padding;
        needheight = CalcOptionsHeight(linewrap_padding);

        position = RectWH(0, ui_view.GetHeight() - needheight, ui_view.GetWidth(), needheight);
    }

    // Apply custom GUI position (except when it's fully custom options rendering)
    if (!usingCustomRendering)
    {
        if (game.Play.dialog_options_gui_x >= 0)
            position.MoveToX(game.Play.dialog_options_gui_x);
        if (game.Play.dialog_options_gui_y >= 0)
            position.MoveToY(game.Play.dialog_options_gui_y);
    }

    newCustomRender = usingCustomRendering && game.Options[OPT_DIALOGOPTIONSAPI] >= 0;
    redrawOnMouseMove = !usingCustomRendering;
    needRedraw = false;
    mouseison = INT32_MIN; // forces a redraw at the first update even if the mouse cursor did not move yet

    CreateOverlay();

    // Disable rest of the game interface while the dialog options are displayed;
    // note that this also disables "overhotspot" labels update
    DisableInterfaceEx(false /* don't change cursor */);
}

void DialogOptionsState::Draw()
{
    Game &game = Game::Current();
    // Since we have our overlay exposed to script, we must check that it was not removed by command
    if (options_overlay_id < 0)
        CreateOverlay();

    ScreenOverlay *options_overlay = get_overlay(options_overlay_id);
    Bitmap *options_bmp = options_overlay->GetImage(EngineServices::Current());

    for (auto &item : items)
        item.Position = Rect();

    if (usingCustomRendering)
    {
        options_bmp->ClearTransparent();

        // Custom dialog options rendering
        ScriptDrawingSurface *dopts_surface = ccDialogOptionsRendering->surfaceToRenderTo;
        dopts_surface->isLinkedBitmapOnly = true;
        dopts_surface->linkedBitmapOnly = options_bmp;
        ccDialogOptionsRendering->surfaceAccessed = false;
        ccDialogOptionsRendering->isInDraw = true;

        game.Scripts.RenderDialogOptions.Params[0].SetScriptObject(ccDialogOptionsRendering, ccDialogOptionsRendering);
        run_function_on_non_blocking_thread(&game.Scripts.RenderDialogOptions);

        if (!ccDialogOptionsRendering->surfaceAccessed)
            debug_script_warn("dialog_options_get_dimensions was implemented, but no dialog_options_render function "
                              "drew anything to the surface");

        if (parserInput)
        {
            parserInput->SetX(ccDialogOptionsRendering->parserTextboxX);
            curyp = ccDialogOptionsRendering->parserTextboxY;
            areawid = ccDialogOptionsRendering->parserTextboxWidth;
            if (areawid == 0)
                areawid = options_bmp->GetWidth();
        }
        ccDialogOptionsRendering->needRepaint = false;
        ccDialogOptionsRendering->isInDraw = false;
    }
    else if (is_textwindow)
    {
        // Draw the text window on background
        options_bmp->Blit(text_window.get());

        // Text window behind the options
        const int padding = game.Guis[game.Options[OPT_DIALOGIFACE]].GetPadding();
        // Ignore the dialog_options_pad_x/y offsets when using a text window
        // because it has its own padding + border gfx
        inner_position = text_window_offset;
        // NOTE: presumably, txoffs and tyoffs are already offset by padding,
        // although it's not entirely reliable, because these calculations are done inside draw_text_window.
        // NOTE: text window is generated around text, so guaranteed to be always padded equally from both sides.
        const int opts_area_width = areawid - (2 * padding + 2);
        curyp = write_dialog_options(options_bmp, inner_position.X, inner_position.Y, opts_area_width, bullet_wid,
                                     game.DialogBulletSprite, bullet_picwid, usingfont, linespacing, forecol, _dtop,
                                     numdisp, mouseison, items);
        if (parserInput)
            parserInput->SetX(inner_position.X);
    }
    else
    {
        // Normal GUI or default surface
        options_bmp->ClearTransparent();

        Bitmap *dst = options_bmp;
        // redraw the background so that anti-alias fonts don't re-alias themselves
        if (game.Options[OPT_DIALOGIFACE] == 0)
        {
            // Default surface
            color_t draw_color = GUI::GetStandardColorForBitmap(16);
            dst->FillRect(RectWH(position.GetSize()), draw_color);
        }
        else
        {
            // Normal GUI
            GUIMain *gui = &game.Guis[game.Options[OPT_DIALOGIFACE]];
            draw_gui_for_dialog_options(dst, gui, 0, 0);
        }
        // FIXME: alignment of the "text options area" here is very broken;
        // probably a combined result of code refactor + attempts to emulate old game behavior.
        // linewrap_padding == game.Play.dialog_options_pad_x + fixed_padding, and this is subtracted
        // from areawid. But then, why the area X is not shifted by fixed_padding?
        const int opts_area_width = areawid - (2 * linewrap_padding + 2) - normal_area_counter_align_pad;
        if (game.Play.dialog_options_textalign == kHAlignLeft)
            inner_position = Point(game.Play.dialog_options_pad_x + line_x_off, game.Play.dialog_options_pad_y);
        else if (game.Play.dialog_options_textalign ==
                 kHAlignRight) // FIXME: following is a hotfix to counter areawid's reduction
            inner_position = Point(areawid - opts_area_width - (game.Play.dialog_options_pad_x + line_x_off),
                                   game.Play.dialog_options_pad_y);
        curyp = write_dialog_options(dst, inner_position.X, inner_position.Y, opts_area_width, bullet_wid,
                                     game.DialogBulletSprite, bullet_picwid, usingfont, linespacing, forecol, _dtop,
                                     numdisp, mouseison, items);

        if (parserInput)
            parserInput->SetX(inner_position.X);
    }

    if (parserInput)
    {
        // Set up the text box, if present
        parserInput->SetY(curyp + game.Options[OPT_DIALOGGAP]);
        parserInput->SetWidth(areawid - 10);
        parserInput->SetTextColor(game.PlayerChar->SpeechColor);
        if (mouseison == DLG_OPTION_PARSER)
            parserInput->SetTextColor(forecol);

        // Left-to-right text direction flag
        const bool ltr_position = (game.Options[OPT_RIGHTLEFTWRITE] == 0);

        parserInput->SetWidth(parserInput->GetWidth() - bullet_wid);
        if (ltr_position)
            parserInput->SetX(parserInput->GetX() + bullet_wid);

        const int parser_x = parserInput->GetX();
        const int parser_y = parserInput->GetY();
        Bitmap *dst = options_bmp;
        if (game.DialogBulletSprite)
        {
            if (ltr_position)
                draw_gui_sprite(dst, game.DialogBulletSprite, parser_x - bullet_wid, parser_y);
            else
                draw_gui_sprite(dst, game.DialogBulletSprite,
                                parser_x + parserInput->GetWidth() + (bullet_wid - bullet_picwid + 1), parser_y);
        }

        parserInput->Draw(dst, parser_x, parser_y);
        parserInput->SetActivated(false);
    }

    // Mark the overlay's image as changed
    options_overlay->MarkImageChanged();
}

bool DialogOptionsState::Run()
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    // Run() can be called in a loop, so keep events going.
    sys_evt_process_pending();

    // Optionally run full game update, otherwise only minimal auto & overlay update
    // NOTE: the redrawing of the dialog options themselves is a bit off here in this code:
    // it happens AFTER we update the rest of the game, and stored on the dialog options
    // texture, which will become updated on screen during the *next* Run().
    // But technically there's no wait between options redraw and next update.
    // The wait is between update and next options redraw... We might benefit from
    // another code restructure here.
    if (runGameLoopsInBackground)
    {
        UpdateGameOnce(false);
    }
    else
    {
        update_polled_stuff();
        update_audio_system_on_game_loop();
        UpdateCursorAndDrawables();
        render_graphics();

        if (!game.Play.fast_forward)
            WaitForNextFrame();
    }

    // Stop the dialog options if it was requested from script
    if (doStop)
        return false;

    needRedraw = false;

    // If there are no displayed options, and we are using standard options render,
    // then bail out as emergency, because otherwise player will be stuck forever.
    if ((numdisp == 0) && !usingCustomRendering)
    {
        debug_script_warn("No dialog options to display, abort dialog");
        return false;
    }

    // For the newer custom options rendering API: run "dialog_options_repexec"
    if (newCustomRender)
    {
        game.Scripts.RunDialogOptionRepExec.Params[0].SetScriptObject(ccDialogOptionsRendering,
                                                                      ccDialogOptionsRendering);
        run_function_on_non_blocking_thread(&game.Scripts.RunDialogOptionRepExec);

        // Stop the dialog options if requested from script
        if (doStop)
            return false;
    }

    // Handle mouse over options
    int mouse_was_over = mouseison;
    mouseison = -1;
    if (newCustomRender)
    {
        // New custom rendering: do not automatically detect option under mouse
    }
    else if (usingCustomRendering)
    {
        // Old custom rendering
        if (game.Play.mouse_input_enabled && position.IsInside(services.Input.MouseX, services.Input.MouseY))
        {
            // Run "dialog_options_get_active"
            game.Scripts.GetDialogOptionUnderCursor.Params[0].SetScriptObject(ccDialogOptionsRendering,
                                                                              ccDialogOptionsRendering);
            run_function_on_non_blocking_thread(&game.Scripts.GetDialogOptionUnderCursor);

            if (!game.Scripts.GetDialogOptionUnderCursor.AtLeastOneImplementationExists)
                quit("!The script function dialog_options_get_active is not implemented. It must be present to use a "
                     "custom dialogue system.");

            // Stop the dialog options if requested from script
            if (doStop)
                return false;

            mouseison = ccDialogOptionsRendering->activeOptionID;
        }
        else
        {
            ccDialogOptionsRendering->activeOptionID = -1;
        }
    }
    else
    {
        // Default rendering: detect option under mouse
        if (game.Play.mouse_input_enabled && Rect(position.Left + inner_position.X, position.Top + inner_position.Y,
                                                  position.Left + inner_position.X + areawid, position.Top + curyp)
                                                 .IsInside(services.Input.MouseX, services.Input.MouseY))
        {
            const int rel_mouse_y = services.Input.MouseY - position.Top;
            mouseison = numdisp - 1;
            for (int i = 0; i < numdisp; ++i)
            {
                if (rel_mouse_y < items[i].Position.Top)
                {
                    mouseison = i - 1;
                    break;
                }
            }
            if ((mouseison < 0) || (mouseison >= numdisp))
                mouseison = -1;
        }
    }

    // Handle mouse over parser
    if (parserInput)
    {
        const int rel_mouse_y = services.Input.MouseY - position.Top;
        if ((rel_mouse_y > parserInput->GetY()) && (rel_mouse_y < parserInput->GetY() + parserInput->GetHeight()))
            mouseison = DLG_OPTION_PARSER;
    }

    // Handle player's input
    RunControls();

    // Stop the dialog options if requested from script
    if (doStop)
        return false;

    // Post user input, processing changes
    if (newCustomRender)
    {
        // New-style custom rendering: check its explicit flag;
        // could be set by setting ActiveOptionID, or calling Update()
        needRedraw |= ccDialogOptionsRendering->needRepaint;
    }

    if (redrawOnMouseMove)
    {
        // Default rendering and old-style custom rendering:
        // test if an active option has changed
        needRedraw |= (mouse_was_over != mouseison);
    }

    // Handle new parser's state
    if (parserInput && parserInput->IsActivated())
    {
        parserActivated = 1;
    }

    // Get if any option has been chosen
    if (chose >= 0)
    { // already have one set by default behavior
    }
    else if (parserActivated)
    {
        // They have selected a custom parser-based option
        if (!parserInput->GetText().IsEmpty() != 0)
        {
            chose = DLG_OPTION_PARSER;
        }
        else
        {
            parserActivated = 0;
            parserInput->SetActivated(false);
        }
    }
    else if (newCustomRender)
    {
        // New custom rendering: see if RunActiveOption was called
        if (ccDialogOptionsRendering->chosenOptionID >= 0)
        {
            chose = ccDialogOptionsRendering->chosenOptionID;
            ccDialogOptionsRendering->chosenOptionID = -1;
        }
    }

    // Finally, if the option has been chosen, then break the options loop
    if (chose >= 0)
        return false;

    // Redraw if needed
    if (needRedraw)
        Draw();

    return true; // continue running loop
}

bool DialogOptionsState::RunControls()
{
    Game &game = Game::Current();
    bool state_handled = false;
    for (InputType type = kInputNone; ags_hasinputevent_ready(type);)
    {
        switch (type)
        {
        case kInputKeyboard:
        {
            KeyInput key_input;
            if (!run_service_key_controls(key_input) || state_handled)
                continue; // handled by engine layer, or resolved
            if (!game.Play.IsIgnoringInput() && RunKey(key_input))
            {
                state_handled = true; // handled
            }
            break;
        }
        case kInputMouse:
        {
            eAGSMouseButton button;
            Point mouse_pos;
            if (!run_service_mb_controls(button, &mouse_pos) || state_handled)
                continue; // handled by engine layer, or resolved
            if (!game.Play.IsIgnoringInput() && RunMouse(button, mouse_pos.X, mouse_pos.Y))
            {
                state_handled = true; // handled
            }
            break;
        }
        default: ags_drop_next_inputevent(); break;
        }
    }
    // Finally handle mouse wheel
    const int wheel = ags_check_mouse_wheel(); // poll always, otherwise it accumulates
    if (!state_handled)
        state_handled = RunMouseWheel(wheel);
    return state_handled;
}

bool DialogOptionsState::RunKey(const KeyInput &key_input)
{
    Game &game = Game::Current();
    const eAGSKeyCode ags_key = key_input.Key;
    if (parserInput)
    {
        // type into the parser
        // TODO: find out what are these key commands, and are these documented?
        if ((ags_key == eAGSKeyCodeF3) || ((ags_key == eAGSKeyCodeSpace) && (parserInput->GetText().GetLength() == 0)))
        {
            // write previous contents into textbox (F3 or Space when box is empty)
            size_t last_len = ustrlen(game.Play.lastParserEntry);
            size_t cur_len = ustrlen(parserInput->GetText().GetCStr());
            // Like F3 in a DOS command line: the typed text is taken to be the start of the previous entry,
            // and only the rest of the previous entry is appended (it is not checked to be a real prefix)
            if (cur_len < last_len)
            {
                const char *entry = game.Play.lastParserEntry;
                // TODO: utility function for advancing N utf-8 chars
                for (size_t i = 0; i < cur_len; ++i)
                    ugetxc(&entry);
                parserInput->SetText(String::FromFormat("%s%s", parserInput->GetText().GetCStr(), entry));
            }
            needRedraw = true;
            return true; // handled
        }
        else if ((key_input.UChar > 0) || (ags_key == eAGSKeyCodeReturn) || (ags_key == eAGSKeyCodeBackspace))
        {
            bool handled = parserInput->OnKeyPress(key_input);
            needRedraw = handled;
            return handled;
        }
    }
    else if (newCustomRender)
    {
        if (key_input.UChar == 0)
        { // "dialog_options_key_press"
            game.Scripts.RunDialogOptionKeyPress.Params[0].SetScriptObject(ccDialogOptionsRendering,
                                                                           ccDialogOptionsRendering);
            game.Scripts.RunDialogOptionKeyPress.Params[1].SetInt32(AGSKeyToScriptKey(key_input.Key));
            game.Scripts.RunDialogOptionKeyPress.Params[2].SetInt32(key_input.Mod);
            run_function_on_non_blocking_thread(&game.Scripts.RunDialogOptionKeyPress);
        }
        if (key_input.UChar > 0)
        { // "dialog_options_text_input"
            game.Scripts.RunDialogOptionTextInput.Params[0].SetScriptObject(ccDialogOptionsRendering,
                                                                            ccDialogOptionsRendering);
            game.Scripts.RunDialogOptionTextInput.Params[1].SetInt32(key_input.UChar);
            run_function_on_non_blocking_thread(&game.Scripts.RunDialogOptionKeyPress);
        }
        return key_input.UChar >= 0;
    }
    // Allow selection of options by keyboard shortcuts
    else if (game.Options[OPT_DIALOGNUMBERED] >= kDlgOptKeysOnly && ags_key >= '1' && ags_key <= '9')
    {
        int key_index = ags_key - '1';
        if (key_index < numdisp)
        {
            chose = items[key_index].OptionID;
            return true; // handled
        }
    }
    return false; // not handled
}

bool DialogOptionsState::RunMouse(eAGSMouseButton button, int x, int y)
{
    Game &game = Game::Current();
    if (button > kMouseNone)
    {
        if (mouseison < 0 && !newCustomRender)
        {
            if (usingCustomRendering)
            {
                game.Scripts.RunDialogOptionMouseClick.Params[0].SetScriptObject(ccDialogOptionsRendering,
                                                                                 ccDialogOptionsRendering);
                game.Scripts.RunDialogOptionMouseClick.Params[1].SetInt32(button);
                run_function_on_non_blocking_thread(&game.Scripts.RunDialogOptionMouseClick);
                needRedraw = game.Scripts.RunDialogOptionMouseClick.AtLeastOneImplementationExists;
            }
        }
        else if (mouseison == DLG_OPTION_PARSER)
        {
            // they clicked the text box
            parserActivated = 1;
        }
        else if (newCustomRender)
        {
            game.Scripts.RunDialogOptionMouseClick.Params[0].SetScriptObject(ccDialogOptionsRendering,
                                                                             ccDialogOptionsRendering);
            game.Scripts.RunDialogOptionMouseClick.Params[1].SetInt32(button);
            game.Scripts.RunDialogOptionMouseClick.Params[2].SetInt32(x);
            game.Scripts.RunDialogOptionMouseClick.Params[3].SetInt32(y);
            run_function_on_non_blocking_thread(&game.Scripts.RunDialogOptionMouseClick);
        }
        else if (usingCustomRendering)
        {
            chose = mouseison;
        }
        else
        {
            chose = items[mouseison].OptionID;
        }
        return true; // always treat handled, any mouse button does the same
    }
    return false; // not handled
}

bool DialogOptionsState::RunMouseWheel(int wheel)
{
    Game &game = Game::Current();
    if ((wheel != 0) && usingCustomRendering)
    {
        game.Scripts.RunDialogOptionMouseClick.Params[0].SetScriptObject(ccDialogOptionsRendering,
                                                                         ccDialogOptionsRendering);
        game.Scripts.RunDialogOptionMouseClick.Params[1].SetInt32((wheel < 0) ? 9 : 8);
        run_function_on_non_blocking_thread(&game.Scripts.RunDialogOptionMouseClick);
        needRedraw = !newCustomRender && game.Scripts.RunDialogOptionMouseClick.AtLeastOneImplementationExists;
        return true; // handled
    }
    return false; // not handled
}

void DialogOptionsState::End()
{
    Game &game = Game::Current();
    // Close custom dialog options
    if (usingCustomRendering)
    {
        game.Scripts.RunDialogOptionClose.Params[0].SetScriptObject(ccDialogOptionsRendering, ccDialogOptionsRendering);
        run_function_on_non_blocking_thread(&game.Scripts.RunDialogOptionClose);
    }

    // Invalidate DialogOptionsRendering and DrawingSurface objects in case they saved its reference in script
    if (ccDialogOptionsRendering->surfaceToRenderTo)
    {
        ccDialogOptionsRendering->surfaceToRenderTo->Invalidate();
        ccReleaseObjectReference(ccGetObjectHandleFromAddress(ccDialogOptionsRendering->surfaceToRenderTo));
    }
    ccDialogOptionsRendering->Reset();
    ccReleaseObjectReference(ccGetObjectHandleFromAddress(ccDialogOptionsRendering));
    ccDialogOptionsRendering = nullptr; // disposed by the managed pool

    invalidate_screen();

    if (parserActivated)
    {
        assert(parserInput);
        snprintf(game.Play.lastParserEntry, MAX_MAXSTRLEN, "%s", parserInput->GetText().GetCStr());
        ParseText(parserInput->GetText().GetCStr());
        chose = CHOSE_TEXTPARSER;
    }

    if (options_overlay_id >= 0)
    {
        remove_screen_overlay(options_overlay_id);
        options_overlay_id = 0;
    }
    parserInput.reset();

    set_cursor_look(curswas);
    // In case it's the QFG4 style dialog, remove the black screen
    game.Play.in_conversation--;
    remove_screen_overlay(OVER_COMPLETE);

    // Re-enabled the game GUI
    EnableInterfaceEx(false /* don't change cursor */);
}

int run_dialog_entry(int dialog_id)
{
    Dialog *dialog_topic = &Game::Current().Dialogs[dialog_id];
    // Run global event kScriptEvent_DialogRun for the startup entry (index 0)
    run_on_event(kScriptEvent_DialogRun, dialog_id, 0);
    return run_dialog_script(dialog_id, dialog_topic->StartEntryPoint, 0);
}

int run_dialog_option(int dialog_id, int dialog_choice, int say_chosen_option, bool run_script)
{
    Game &game = Game::Current();
    Dialog *dialog_topic = &game.Dialogs[dialog_id];
    assert(dialog_choice >= 0 && static_cast<uint32_t>(dialog_choice) < dialog_topic->GetOptionCount());
    int &option_flags = dialog_topic->Options[dialog_choice].Flags;
    const String &option_name = dialog_topic->Options[dialog_choice].Text;

    // Run global event kScriptEvent_DialogRun for the new option
    run_on_event(kScriptEvent_DialogRun, dialog_id, dialog_choice + 1);

    option_flags |= DFLG_HASBEENCHOSEN;
    bool say_option = false;
    if (say_chosen_option == SAYCHOSEN_YES)
    {
        say_option = true;
    }
    else if (say_chosen_option == SAYCHOSEN_USEFLAG)
    {
        say_option = ((option_flags & DFLG_NOREPEAT) == 0);
    }

    // Optionally "say" the option's text
    if (say_option)
        DisplaySpeech(get_translation(option_name.GetCStr()), game.PlayerCharacterID);

    // Run the option script
    if (run_script)
        return run_dialog_script(dialog_id, dialog_topic->Options[dialog_choice].EntryPoint, dialog_choice + 1);

    return 0; // no script, bail out
}

int show_dialog_options(int dialog_id, bool run_game_loops_in_background)
{
    Game &game = Game::Current();
    if ((dialog_id < 0) || (dialog_id >= game.DialogCount))
    {
        quit("!RunDialog: invalid dialog number specified");
    }

    can_run_delayed_command();

    Dialog *topic = &game.Dialogs[dialog_id];

    // First test if there are enough valid options to run DialogOptionsState
    int opt_count = 0;
    int last_opt = -1;
    for (uint32_t i = 0; i < topic->GetOptionCount(); ++i)
    {
        if ((topic->Options[i].Flags & DFLG_ON) != 0)
        {
            last_opt = i;
            opt_count++;
        }
    }

    if (opt_count < 1)
    {
        debug_script_warn("Dialog: all options have been turned off, stopping dialog.");
        return -1;
    }
    // Don't display the options if there is only one and the parser is not enabled.
    const bool has_parser = (topic->Flags & DTFLG_SHOWPARSER) && (game.Play.disable_dialog_parser == 0);
    if (!has_parser && (opt_count == 1) && !game.Play.show_single_dialog_option)
    {
        return last_opt; // only one choice, so select it
    }

    // Run the global DialogOptionsOpen event
    run_on_event(kScriptEvent_DialogOptionsOpen, dialog_id);

    dialogOpts = std::make_unique<DialogOptionsState>(topic, dialog_id, run_game_loops_in_background);
    dialogOpts->Show();

    // Run the global DialogOptionsClose event
    run_on_event(kScriptEvent_DialogOptionsClose, dialog_id, dialogOpts->GetChosenOption());

    const int chosen = dialogOpts->GetChosenOption();
    dialogOpts = {};
    return chosen;
}

// Dialog execution state
// TODO: reform into GameState implementation, similar to DialogOptionsState!
class DialogExec
{
public:
    DialogExec(int start_dialog_id, int start_opt = 0) : _dlgNum(start_dialog_id), _startOpt(start_opt) {}

    // Tells if the dialog is either processing or ended on the start entry
    // TODO: possibly a hack, investigate if it's possible to do without this
    bool IsFirstEntry() const { return _isFirstEntry; }
    int GetDlgNum() const { return _dlgNum; }
    int GetExecutedOption() const { return _executedOption; }
    bool AreOptionsDisplayed() const { return _areOptionsDisplayed; }

    // TODO: a hack, see also the comment below
    ScriptPosition &GetSavedDialogRequestScPos() { return _savedDialogRequestScriptPos; }

    // Runs Dialog state
    void Run();
    // Requests a start of another dialog after current dialog script is completed.
    // This request is scheduled and performed as soon as dialog state receives control.
    // This overrides the dialog script's return value.
    void GotoDialog(int dialog_id, int start_opt = 0);
    // Request the Dialog state to stop.
    // This request is scheduled and performed as soon as dialog state receives control.
    // This overrides the dialog script's return value.
    void Stop();

private:
    int HandleDialogResult(int result);

    int _dlgNum = -1;
    int _dlgWas = -1;
    int _startOpt = -1;
    // Tells if the dialog process is running the very first entry overall
    // (not just the starting entry of any topic)
    // TODO: do_conversation() uses this to remove the full-screen overlay and decrement game.Play.in_conversation
    // when the dialog stopped on its start entry; check if that is still needed, because the matching
    // increment is made only when the dialog options are shown
    bool _isFirstEntry = true;
    // Dialog topics history, used by "goto-previous" command
    std::stack<int> _topicHist;
    int _executedOption = -1;           // option which is currently run (or -1)
    bool _areOptionsDisplayed = false;  // if dialog options are displayed on screen
    int _doRunDialog = RUN_DIALOG_STAY; // scheduled goto dialog
    bool _doStop = false;               // scheduled stop

    // A position in script saved by certain API function calls in "dialog_request" callback;
    // used purely for error reporting when the script has 2+ calls to gamestate-changing
    // functions such as StartDialog or ChangeRoom.
    // TODO: this is horrible, review this and make consistent with error reporting
    // for regular calls in normal script.
    ScriptPosition _savedDialogRequestScriptPos;
};

int DialogExec::HandleDialogResult(int result)
{
    // Stop the dialog if requested
    if (_doStop)
        return RUN_DIALOG_STOP_DIALOG;

    if ((_doRunDialog >= 0) || (_doRunDialog == RUN_DIALOG_GOTO_PREVIOUS))
    {
        result = _doRunDialog;
    }

    // Handle goto-previous, see if there's any previous dialog in history
    if (result == RUN_DIALOG_GOTO_PREVIOUS)
    {
        if (_topicHist.size() == 0)
            return RUN_DIALOG_STOP_DIALOG;
        result = _topicHist.top();
        _topicHist.pop();
    }
    // Continue to the next dialog
    if (result >= 0)
    {
        // Save the old topic number in the history, and switch to the new one (unless restarting same dialog)
        if (_topicHist.size() == 0 || _topicHist.top() != _dlgNum)
            _topicHist.push(_dlgNum);
        _dlgNum = result;
        return _dlgNum;
    }
    return result;
}

void DialogExec::Run()
{
    Game &game = Game::Current();
    _doStop = false;

    // Run option index counts index 0 as a starting entry, actual options begin with index 1
    int run_option = _startOpt;
    _startOpt = -1;            // reset starting option
    bool show_options = false; // begin with running starting entry

    while (_dlgNum >= 0)
    {
        if (_dlgNum < 0 || _dlgNum >= game.DialogCount)
            quitprintf("!RunDialog: invalid dialog number specified: %d", _dlgNum);

        // current dialog object
        Dialog *topic = &game.Dialogs[_dlgNum];
        int result = 0; // dialog execution result
        // reset global dialog flow instruction to default;
        // unless this is changed by a explicit script command, DialogExec will refer
        // to the result of processing a next dialog entry or chosen option.
        _doRunDialog = RUN_DIALOG_STAY;

        // Show current dialog's options if necessary
        if (show_options)
        {
            show_options = false;
            _doRunDialog = RUN_DIALOG_STAY;
            _areOptionsDisplayed = true;
            run_option = show_dialog_options(_dlgNum, (game.Options[OPT_RUNGAMEDLGOPTS] != 0));
            // turn into 1-based index; should never become 0 here as starting entry cannot be selected;
            // if show_dialog_options() returned -1, that means end of dialog
            if (run_option >= 0)
                run_option++;
            _areOptionsDisplayed = false;

            // Stop the dialog if was requested from custom options script
            if (_doStop)
                return;
        }

        // Run requested option, or a dialog request in case of a text parser commit
        if (run_option == CHOSE_TEXTPARSER)
        {
            game.Speech.SaidSpeechLine = 0;
            result = run_dialog_request(_dlgNum);
            if (game.Speech.SaidSpeechLine > 0)
            {
                // fix the problem with the close-up face remaining on screen
                // TODO: find a better solution rather than disable/enable iface?
                // this causes all gui texture reupdate twice, and other things...
                DisableInterface();
                UpdateGameOnce(); // redraw the screen to make sure it looks right
                EnableInterface();
                set_cursor_look(game.GetModePointer());
            }
        }
        else if (run_option >= 0)
        {
            // chose some option - handle it and run its script
            _executedOption = run_option;
            if (run_option == 0)
                result = run_dialog_entry(_dlgNum);
            else
                result = run_dialog_option(_dlgNum, run_option - 1, SAYCHOSEN_USEFLAG, true /* run script */);
            _dlgWas = _dlgNum; // remember last dialog topic
            _executedOption = -1;
        }
        else
        {
            // no option chosen? - stop the dialog
            return;
        }

        // Handle the dialog option's result
        result = HandleDialogResult(result);
        if (result == RUN_DIALOG_STOP_DIALOG)
            return; // stop the dialog
        _isFirstEntry = false;
        // Show same dialog's options again
        if (result == RUN_DIALOG_STAY)
            show_options = true;
        // Next or previous dialog: setup starting entry index
        else
            run_option = _startOpt >= 0 ? _startOpt : 0;
    }
}

void DialogExec::GotoDialog(int dialog_id, int start_opt)
{
    _doRunDialog = dialog_id;
    _startOpt = start_opt;
}

void DialogExec::Stop()
{
    _doStop = true;
}

void do_conversation(int dialog_id, int start_opt)
{
    assert(dialogExec == nullptr);
    if (dialogExec)
    {
        Debug::Printf(kDbgMsg_Error, "ERROR: tried to start a new dialog state while a dialog state is running.");
        return;
    }

    EndSkippingUntilCharStops();

    // Run the global DialogStart event
    run_on_event(kScriptEvent_DialogStart, dialog_id);

    dialogExec = std::make_unique<DialogExec>(dialog_id, start_opt);
    dialogExec->Run();
    // TODO: this is done only when the dialog never went past its start entry; check if it is safe (and
    // needed) to do always, regardless of the number of iterations
    if (dialogExec->IsFirstEntry())
    {
        // bail out from first startup script
        remove_screen_overlay(OVER_COMPLETE);
        Game::Current().Play.in_conversation--;
    }

    // Run the global DialogStop event; NOTE: _dlgNum may be different in the end
    run_on_event(kScriptEvent_DialogStop, dialogExec->GetDlgNum());
    dialogExec = {};

    set_default_cursor_look();
}

bool is_in_dialog()
{
    return dialogExec != nullptr;
}

bool is_in_dialogoptions()
{
    return dialogOpts != nullptr;
}

bool is_dialog_executing_script()
{
    return dialogExec && (dialogExec->GetExecutedOption() >= 0) &&
           scriptExecutor; // a dialog script cannot be running without the script executor
}

// TODO: this is ugly: the dialog script's goto request reaches the state object through a global function
void set_dialog_result_goto(int dialog_id, int start_opt)
{
    assert(is_dialog_executing_script());
    if (is_dialog_executing_script())
    {
        dialogExec->GotoDialog(dialog_id, start_opt);
    }
}

void set_dialog_result_stop()
{
    assert(is_dialog_executing_script());
    if (is_dialog_executing_script())
    {
        dialogExec->Stop();
    }
}

bool handle_state_change_in_dialog_request(const char *api_name, int request_result, int start_opt)
{
    Game &game = Game::Current();
    // Test if we are inside a dialog state AND dialog_request callback
    if ((dialogExec == nullptr) || (game.Play.stop_dialog_at_end == DIALOG_NONE))
    {
        return false; // not handled, process command as normal
    }

    // Test if dialog result was not set yet
    if (game.Play.stop_dialog_at_end == DIALOG_RUNNING)
    {
        game.Play.stop_dialog_at_end = request_result;
        get_script_position(dialogExec->GetSavedDialogRequestScPos());
        if (request_result == DIALOG_STOP)
            dialogExec->Stop();
        else if (request_result >= DIALOG_NEWTOPIC)
            dialogExec->GotoDialog(request_result - DIALOG_NEWTOPIC, start_opt);
    }
    else
    {
        debug_script_warn("!%s: more than one NewRoom/RunDialog/StopDialog requests within a dialog '%s' (%d), "
                          "following one(s) will be ignored\n\tfirst was made in \"%s\", line %d",
                          api_name, game.Dialogs[dialogExec->GetDlgNum()].ScriptName.GetCStr(), dialogExec->GetDlgNum(),
                          dialogExec->GetSavedDialogRequestScPos().Section.GetCStr(),
                          dialogExec->GetSavedDialogRequestScPos().Line);
    }
    return true; // handled, state change will be taken care of by a dialog script
}

void schedule_dialog_stop()
{
    // NOTE: dialog options may be displayed with Dialog.DisplayOptions() too
    assert(dialogExec || dialogOpts);
    if (dialogExec)
        dialogExec->Stop();
    if (dialogOpts)
        dialogOpts->Stop();
}

void shutdown_dialog_state()
{
    dialogExec = {};
    dialogOpts = {};
}

// end dialog manager
//=============================================================================

bool DialogOptions_GetHasCustomRender()
{
    if (!dialogOpts)
        return false;

    return dialogOpts->HasCustomRender();
}

int DialogOptions_GetItemCount()
{
    if (!dialogOpts)
        return 0;

    return static_cast<int>(dialogOpts->GetItems().size());
}

int DialogOptions_GetItemOptionID(int index)
{
    if (!dialogOpts || index < 0 || static_cast<uint32_t>(index) >= dialogOpts->GetItems().size())
        return 0;

    return dialogOpts->GetItems()[index].OptionID + 1; // 1-based script index
}

int DialogOptions_GetItemX(int index)
{
    if (!dialogOpts || index < 0 || static_cast<uint32_t>(index) >= dialogOpts->GetItems().size())
        return 0;

    return dialogOpts->GetItems()[index].Position.Left;
}

int DialogOptions_GetItemY(int index)
{
    if (!dialogOpts || index < 0 || static_cast<uint32_t>(index) >= dialogOpts->GetItems().size())
        return 0;

    return dialogOpts->GetItems()[index].Position.Top;
}

int DialogOptions_GetItemWidth(int index)
{
    if (!dialogOpts || index < 0 || static_cast<uint32_t>(index) >= dialogOpts->GetItems().size())
        return 0;

    return dialogOpts->GetItems()[index].Position.GetWidth();
}

int DialogOptions_GetItemHeight(int index)
{
    if (!dialogOpts || index < 0 || static_cast<uint32_t>(index) >= dialogOpts->GetItems().size())
        return 0;

    return dialogOpts->GetItems()[index].Position.GetHeight();
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "game/dynobj/cc_dialog.h"
#include "game/dynobj/script_string.h"
#include "script/script_runtime.h"

ScriptDialog *Dialog_GetByName(const char *name)
{
    return static_cast<ScriptDialog *>(ccGetScriptObjectAddress(name, ccDynamicDialog.GetType()));
}

void Dialog_Stop()
{
    StopDialog();
}

ScriptDialog *Dialog_GetCurrentDialog()
{
    return dialogExec ? &Game::Current().ScDialogs[dialogExec->GetDlgNum()] : nullptr;
}

int Dialog_GetExecutedOption()
{
    return dialogExec ? dialogExec->GetExecutedOption() : -1;
}

ScriptOverlay *DialogOptions_GetOverlay()
{
    return dialogOpts ? (ScriptOverlay *)ccGetObjectAddressFromHandle(dialogOpts->GetOverlayHandle()) : nullptr;
}

bool Dialog_GetAreOptionsDisplayed()
{
    return is_in_dialogoptions();
}
