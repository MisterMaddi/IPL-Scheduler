#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Constants
#define MAX_RESTRICTION_DAYS 50
#define MAX_TEAMS 10
#define OVERS_PER_MATCH 20
#define MAX_MATCHES 100
#define MAX_PLAYERS_PER_TEAM 15
#define MAX_BUFFER_SIZE 1024
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 40
#define INPUT_BOX_WIDTH 300
#define INPUT_BOX_HEIGHT 40

// Enum for application states
typedef enum {
    MENU,
    TEAM_INPUT,
    RESTRICTION_INPUT,
    DATE_INPUT,
    SCHEDULE_VIEW,
    POINTS_VIEW,
    PLAYOFF_INPUT,
    AWARDS_VIEW,
    EXIT
} GameState;

// Structures
typedef struct {
    char name[50];
    char home_stadium[50];
    int points;
    int matches_played;
    int matches_won;
    int matches_lost;
    float net_run_rate;
    Color color;  // Team color
} Team;

typedef struct {
    int home_index;
    int away_index;
    struct tm match_date;
    char stadium[50];
    int home_runs;
    int away_runs;
    int is_cancelled;
} Match;

typedef struct {
    char name[50];
    int runs;
    int wickets;
    int team_index;

    // Attributes:
    int fours;
    int sixes;
    float strike_rate;
    int catches;
    int run_outs;
    char role[15];
    float economy;
    bool is_foreign;
    bool is_captain;
} Player;

// Global variables
struct tm restricted_days[MAX_RESTRICTION_DAYS];
int num_restricted_days = 0;
Team teams[MAX_TEAMS];
int num_teams = 0;
Match matches[MAX_MATCHES];
int num_matches = 0;
int num_players = 0;
int team_last_match_day[MAX_TEAMS];
Player players[MAX_TEAMS][MAX_PLAYERS_PER_TEAM];
char input_buffer[MAX_BUFFER_SIZE] = {0};
int cursor_position = 0;
GameState current_state = MENU;
struct tm start_date = {0};
struct tm end_date = {0};
int active_input_field = 0;
int playoffs_stage = 0;
int q1_home, q1_away, elim_home, elim_away, q2_home, q2_away, final_home, final_away;
int q1_winner, q1_loser, elim_winner, q2_winner, final_winner;
float scroll_position = 0.0f;
int text_input_active = 0;
char current_input[50] = {0};
bool show_message = false;
char notification_message[128] = {0};
float notification_timer = 0.0f;
char start_day_buffer[3] = {0};
char start_month_buffer[3] = {0};
char start_year_buffer[5] = {0};
char end_day_buffer[3] = {0};
char end_month_buffer[3] = {0};
char end_year_buffer[5] = {0};
bool awards_generated = false;

// Textures and sounds
Texture2D background_texture;
Texture2D ipl_logo;
Texture2D trophy_texture;
Texture2D orange_cap_texture;
Texture2D purple_cap_texture;
Music background_music;
Sound click_sound;
Font cricket_font;

// Forward declarations
void initialize_game(void);
void update_game(void);
void draw_game(void);
void unload_resources(void);
void input_teams_screen(void);
void input_restrictions_screen(void);
void input_dates_screen(void);
void schedule_view_screen(void);
void points_view_screen(void);
void playoff_input_screen(void);
void awards_view_screen(void);
void menu_screen(void);
void generate_schedule_without_consecutive_matches(void);
int is_restricted_day(struct tm date);
void cancel_matches_due_to_rain(void);
void display_points_table(void);
void generate_players(void);
void show_notification(const char* text);
Rectangle get_text_bounds(const char* text, int font_size, int x, int y);
bool button_clicked(Rectangle bounds, const char* text, int font_size);
void update_message(void);
void adjust_team_stats_after_match(int home_index, int away_index, int home_runs, int away_runs);
int get_day_number(struct tm date);
void generate_random_team_colors(void);

int main(void) {
    srand(time(NULL));
   
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "IPL Tournament Manager");
    SetTargetFPS(60);
   
    // Initialize audio
    InitAudioDevice();
   
    // Initialize game assets and data
    initialize_game();
   
    // Main game loop
    while (!WindowShouldClose()) {
        update_game();
        draw_game();
    }
   
    // Clean up resources
    unload_resources();
    CloseAudioDevice();
    CloseWindow();
   
    return 0;
}

void update_game(void) {
    // Update music stream
    UpdateMusicStream(background_music);
   
    // Update message display timer
    update_message();
   
    // Handle state-specific updates
    switch (current_state) {
        case MENU:
            // Menu interactions handled in menu_screen()
            break;
        case TEAM_INPUT:
            // Team input handled in input_teams_screen()
            break;
        case RESTRICTION_INPUT:
            // Restriction input handled in input_restrictions_screen()
            break;
        case DATE_INPUT:
            // Date input handled in input_dates_screen()
            break;
        case SCHEDULE_VIEW:
            // Scroll handling for schedule view
            scroll_position -= GetMouseWheelMove() * 20.0f;
            if (scroll_position < 0) scroll_position = 0;
            break;
        case POINTS_VIEW:
            // Points table view handling
            break;
        case PLAYOFF_INPUT:
            // Playoff inputs handled in playoff_input_screen()
            break;
        case AWARDS_VIEW:
            // Awards view handling
            break;
        case EXIT:
            CloseWindow();
            break;
    }
}

void draw_game(void) {
    BeginDrawing();
    ClearBackground(BLACK);
   
    // Draw background
    DrawTexture(background_texture, 0, 0, WHITE);
   
    // Draw state-specific content
    switch (current_state) {
        case MENU:
            menu_screen();
            break;
        case TEAM_INPUT:
            input_teams_screen();
            break;
        case RESTRICTION_INPUT:
            input_restrictions_screen();
            break;
        case DATE_INPUT:
            input_dates_screen();
            break;
        case SCHEDULE_VIEW:
            schedule_view_screen();
            break;
        case POINTS_VIEW:
            points_view_screen();
            break;
        case PLAYOFF_INPUT:
            playoff_input_screen();
            break;
        case AWARDS_VIEW:
            awards_view_screen();
            break;
        case EXIT:
            // Should never reach here as we close the window
            break;
    }
   
    // Draw IPL logo at bottom right corner
    DrawTexture(ipl_logo, SCREEN_WIDTH - ipl_logo.width - 20, SCREEN_HEIGHT - ipl_logo.height - 20, WHITE);
   
    // Draw any active message
    if (show_message) {
        int textWidth = MeasureText(notification_message, 20);
        DrawRectangle(SCREEN_WIDTH/2 - textWidth/2 - 10, 40, textWidth + 20, 40, ColorAlpha(BLACK, 0.7f));
        DrawText(notification_message, SCREEN_WIDTH/2 - textWidth/2, 50, 20, WHITE);
    }
   
    EndDrawing();
}

void unload_resources(void) {
    // Unload textures
    UnloadTexture(background_texture);
    UnloadTexture(ipl_logo);
    UnloadTexture(trophy_texture);
    UnloadTexture(orange_cap_texture);
    UnloadTexture(purple_cap_texture);
   
    // Unload font
    UnloadFont(cricket_font);
   
    // Unload audio
    UnloadMusicStream(background_music);
    UnloadSound(click_sound);
}

void menu_screen(void) {
    int title_font_size = 40;
    char title_text[] = "IPL Tournament Manager";
    int title_width = MeasureText(title_text, title_font_size);
   
    // Draw title
    DrawText(title_text, SCREEN_WIDTH/2 - title_width/2, 100, title_font_size, GOLD);
   
    // Draw menu buttons
    if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 220, BUTTON_WIDTH, BUTTON_HEIGHT}, "New Tournament", 20)) {
        current_state = TEAM_INPUT;
        PlaySound(click_sound);
    }
   
    if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 280, BUTTON_WIDTH, BUTTON_HEIGHT}, "Exit", 20)) {
        current_state = EXIT;
        PlaySound(click_sound);
    }
   
    // Draw IPL season text
    DrawText("IPL 2025", SCREEN_WIDTH/2 - MeasureText("IPL 2025", 30)/2, 170, 30, WHITE);
}

bool button_clicked(Rectangle bounds, const char* text, int font_size) {
    bool clicked = false;
    Vector2 mouse_pos = GetMousePosition();
   
    // Check if mouse is hovering over button
    bool hover = CheckCollisionPointRec(mouse_pos, bounds);
   
    // Draw button with different colors based on hover and click states
    DrawRectangleRec(bounds, hover ? GOLD : DARKBLUE);
    DrawRectangleLinesEx(bounds, 2, WHITE);
   
    // Center text on button
    Vector2 text_pos = {
        bounds.x + bounds.width/2 - MeasureText(text, font_size)/2,
        bounds.y + bounds.height/2 - font_size/2
    };
    DrawText(text, text_pos.x, text_pos.y, font_size, WHITE);
   
    // Check if button was clicked
    if (hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        clicked = true;
    }
   
    return clicked;
}

Rectangle get_text_bounds(const char* text, int font_size, int x, int y) {
    int width = MeasureText(text, font_size);
    int height = font_size;
    return (Rectangle){x, y, width, height};
}

void input_teams_screen(void) {
    static int team_count_input = 6;  // Default to 6 teams
    static int current_team = 0;
    static bool entering_team_name = false;
    static bool entering_stadium_name = false;
    static char team_name_buffer[50] = {0};
    static char stadium_name_buffer[50] = {0};
   
    // Draw title
    DrawText("Team Setup", SCREEN_WIDTH/2 - MeasureText("Team Setup", 30)/2, 50, 30, WHITE);
   
    // Draw team count selector
    DrawText("Number of Teams:", 400, 120, 20, WHITE);
   
    // Draw minus button
    if (button_clicked((Rectangle){630, 120, 30, 30}, "-", 20) && team_count_input > 4) {
        team_count_input--;
        PlaySound(click_sound);
    }
   
    // Draw team count
    DrawText(TextFormat("%d", team_count_input), 675, 120, 20, WHITE);
   
    // Draw plus button
    if (button_clicked((Rectangle){700, 120, 30, 30}, "+", 20) && team_count_input < MAX_TEAMS) {
        team_count_input++;
        PlaySound(click_sound);
    }
   
    if (current_team < team_count_input) {
        // Draw team input form
        DrawText(TextFormat("Team %d of %d", current_team + 1, team_count_input),
                SCREEN_WIDTH/2 - MeasureText(TextFormat("Team %d of %d", current_team + 1, team_count_input), 20)/2, 180, 20, WHITE);
       
        // Team name input
        DrawText("Team Name:", 400, 220, 20, WHITE);
        Rectangle name_rect = {400, 250, 400, 40};
        DrawRectangleRec(name_rect, entering_team_name ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(name_rect, 2, DARKBLUE);
        DrawText(team_name_buffer, name_rect.x + 10, name_rect.y + 10, 20, BLACK);
       
        if (CheckCollisionPointRec(GetMousePosition(), name_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            entering_team_name = true;
            entering_stadium_name = false;
        }
       
        // Stadium name input
        DrawText("Home Stadium:", 400, 320, 20, WHITE);
        Rectangle stadium_rect = {400, 350, 400, 40};
        DrawRectangleRec(stadium_rect, entering_stadium_name ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(stadium_rect, 2, DARKBLUE);
        DrawText(stadium_name_buffer, stadium_rect.x + 10, stadium_rect.y + 10, 20, BLACK);
       
        if (CheckCollisionPointRec(GetMousePosition(), stadium_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            entering_stadium_name = true;
            entering_team_name = false;
        }
       
        // Handle text input
        if (entering_team_name) {
            int key = GetCharPressed();
            while (key > 0) {
                // Only allow valid characters for team name
                if ((key >= 32) && (key <= 125) && (strlen(team_name_buffer) < 49)) {
                    team_name_buffer[strlen(team_name_buffer)] = (char)key;
                    team_name_buffer[strlen(team_name_buffer)] = '\0';
                }
                key = GetCharPressed();
            }
           
            // Handle backspace
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(team_name_buffer) > 0) {
                team_name_buffer[strlen(team_name_buffer) - 1] = '\0';
            }
        }
       
        if (entering_stadium_name) {
            int key = GetCharPressed();
            while (key > 0) {
                // Only allow valid characters for stadium name
                if ((key >= 32) && (key <= 125) && (strlen(stadium_name_buffer) < 49)) {
                    stadium_name_buffer[strlen(stadium_name_buffer)] = (char)key;
                    stadium_name_buffer[strlen(stadium_name_buffer)] = '\0';
                }
                key = GetCharPressed();
            }
           
            // Handle backspace
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(stadium_name_buffer) > 0) {
                stadium_name_buffer[strlen(stadium_name_buffer) - 1] = '\0';
            }
        }
       
        // "Next" button
        if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 450, BUTTON_WIDTH, BUTTON_HEIGHT}, "Next", 20)) {
            // Validate input
            if (strlen(team_name_buffer) > 0 && strlen(stadium_name_buffer) > 0) {
                // Save team data
                strcpy(teams[current_team].name, team_name_buffer);
                strcpy(teams[current_team].home_stadium, stadium_name_buffer);
                teams[current_team].points = 0;
                teams[current_team].matches_played = 0;
                teams[current_team].matches_won = 0;
                teams[current_team].matches_lost = 0;
                teams[current_team].net_run_rate = 0.0f;
               
                // Move to next team
                current_team++;
               
                // Clear buffers
                memset(team_name_buffer, 0, sizeof(team_name_buffer));
                memset(stadium_name_buffer, 0, sizeof(stadium_name_buffer));
                entering_team_name = false;
                entering_stadium_name = false;
               
                PlaySound(click_sound);
               
                // If all teams entered, move to next screen
                if (current_team >= team_count_input) {
                    num_teams = team_count_input;
                    generate_random_team_colors();
                    generate_players();
                    current_state = RESTRICTION_INPUT;
                }
            } else {
                show_notification("Please enter both team name and stadium");
            }
        }
    }
   
    // Back button
    if (button_clicked((Rectangle){50, SCREEN_HEIGHT - 70, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back to Menu", 20)) {
        current_team = 0;
        memset(team_name_buffer, 0, sizeof(team_name_buffer));
        memset(stadium_name_buffer, 0, sizeof(stadium_name_buffer));
        entering_team_name = false;
        entering_stadium_name = false;
        current_state = MENU;
        PlaySound(click_sound);
    }
}

void generate_random_team_colors(void) {
    // Define team colors - IPL-style bright colors
    Color team_colors[] = {
        (Color){0, 97, 255, 255},     // Mumbai blue
        (Color){255, 204, 0, 255},    // Chennai yellow
        (Color){220, 20, 60, 255},    // Bangalore red
        (Color){128, 0, 128, 255},    // Kolkata purple
        (Color){255, 105, 180, 255},  // Pink
        (Color){255, 69, 0, 255},     // Orange
        (Color){0, 128, 0, 255},      // Green
        (Color){70, 130, 180, 255},   // Steel blue
        (Color){218, 165, 32, 255},   // Golden rod
        (Color){148, 0, 211, 255}     // Violet
    };
   
    // Shuffle colors array
    for (int i = 0; i < MAX_TEAMS; i++) {
        int j = rand() % MAX_TEAMS;
        Color temp = team_colors[i];
        team_colors[i] = team_colors[j];
        team_colors[j] = temp;
    }
   
    // Assign colors to teams
    for (int i = 0; i < num_teams; i++) {
        teams[i].color = team_colors[i];
    }
}

void input_restrictions_screen(void) {
    static int restriction_count = 0;
    static bool entering_day = false;
    static bool entering_month = false;
    static bool entering_year = false;
    static char day_buffer[3] = {0};
    static char month_buffer[3] = {0};
    static char year_buffer[5] = {0};
    static int current_restriction = 0;
   
    // Draw title
    DrawText("Restriction Days Setup", SCREEN_WIDTH/2 - MeasureText("Restriction Days Setup", 30)/2, 50, 30, WHITE);
   
    // Draw instruction
    DrawText("Enter dates when matches can't be played (e.g., for rain or events)",
            SCREEN_WIDTH/2 - MeasureText("Enter dates when matches can't be played (e.g., for rain or events)", 20)/2, 100, 20, WHITE);
   
    // Draw restriction count selector
    DrawText("Number of Restriction Days:", 400, 150, 20, WHITE);
   
    // Draw minus button
    if (button_clicked((Rectangle){630, 250, 30, 30}, "-", 20) && restriction_count > 0) {
        restriction_count--;
        PlaySound(click_sound);
    }
   
    // Draw restriction count
    DrawText(TextFormat("%d", restriction_count), 675, 250, 20, WHITE);
   
    // Draw plus button
    if (button_clicked((Rectangle){700, 250, 30, 30}, "+", 20) && restriction_count < MAX_RESTRICTION_DAYS) {
        restriction_count++;
        PlaySound(click_sound);
    }
   
    if (current_restriction < restriction_count) {
        // Draw restriction day input form
        DrawText(TextFormat("Restriction Day %d of %d", current_restriction + 1, restriction_count),
                SCREEN_WIDTH/2 - MeasureText(TextFormat("Restriction Day %d of %d", current_restriction + 1, restriction_count), 20)/2, 200, 20, WHITE);
       
        // Day input
        DrawText("Day:", 400, 250, 20, WHITE);
        Rectangle day_rect = {400, 280, 80, 40};
        DrawRectangleRec(day_rect, entering_day ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(day_rect, 2, DARKBLUE);
        DrawText(day_buffer, day_rect.x + 10, day_rect.y + 10, 20, BLACK);
       
        if (CheckCollisionPointRec(GetMousePosition(), day_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            entering_day = true;
            entering_month = false;
            entering_year = false;
        }
       
        // Month input
        DrawText("Month:", 500, 250, 20, WHITE);
        Rectangle month_rect = {500, 280, 80, 40};
        DrawRectangleRec(month_rect, entering_month ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(month_rect, 2, DARKBLUE);
        DrawText(month_buffer, month_rect.x + 10, month_rect.y + 10, 20, BLACK);
       
        if (CheckCollisionPointRec(GetMousePosition(), month_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            entering_day = false;
            entering_month = true;
            entering_year = false;
        }
       
        // Year input
        DrawText("Year:", 600, 250, 20, WHITE);
        Rectangle year_rect = {600, 280, 100, 40};
        DrawRectangleRec(year_rect, entering_year ? LIGHTGRAY : WHITE);
        DrawRectangleLinesEx(year_rect, 2, DARKBLUE);
        DrawText(year_buffer, year_rect.x + 10, year_rect.y + 10, 20, BLACK);
       
        if (CheckCollisionPointRec(GetMousePosition(), year_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            entering_day = false;
            entering_month = false;
            entering_year = true;
        }
       
        // Handle text input
        if (entering_day) {
            int key = GetCharPressed();
            while (key > 0) {
                // Only allow digits for day
                if ((key >= '0') && (key <= '9') && (strlen(day_buffer) < 2)) {
                    day_buffer[strlen(day_buffer)] = (char)key;
                    day_buffer[strlen(day_buffer)] = '\0';
                }
                key = GetCharPressed();
            }
           
            // Handle backspace
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(day_buffer) > 0) {
                day_buffer[strlen(day_buffer) - 1] = '\0';
            }
        }
       
        if (entering_month) {
            int key = GetCharPressed();
            while (key > 0) {
                // Only allow digits for month
                if ((key >= '0') && (key <= '9') && (strlen(month_buffer) < 2)) {
                    month_buffer[strlen(month_buffer)] = (char)key;
                    month_buffer[strlen(month_buffer)] = '\0';
                }
                key = GetCharPressed();
            }
           
            // Handle backspace
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(month_buffer) > 0) {
                month_buffer[strlen(month_buffer) - 1] = '\0';
            }
        }
       
        if (entering_year) {
            int key = GetCharPressed();
            while (key > 0) {
                // Only allow digits for year
                if ((key >= '0') && (key <= '9') && (strlen(year_buffer) < 4)) {
                    year_buffer[strlen(year_buffer)] = (char)key;
                    year_buffer[strlen(year_buffer)] = '\0';
                }
                key = GetCharPressed();
            }
           
            // Handle backspace
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(year_buffer) > 0) {
                year_buffer[strlen(year_buffer) - 1] = '\0';
            }
        }
       
        // "Add" button
        if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 350, BUTTON_WIDTH, BUTTON_HEIGHT}, "Add", 20)) {
            // Validate input
            if (strlen(day_buffer) > 0 && strlen(month_buffer) > 0 && strlen(year_buffer) > 0) {
                int day = atoi(day_buffer);
                int month = atoi(month_buffer);
                int year = atoi(year_buffer);
               
                // Basic validation
                if (day >= 1 && day <= 31 && month >= 1 && month <= 12 && year >= 2023) {
                    // Save restriction day
                    restricted_days[current_restriction].tm_mday = day;
                    restricted_days[current_restriction].tm_mon = month - 1;
                    restricted_days[current_restriction].tm_year = year - 1900;
                    mktime(&restricted_days[current_restriction]); // Normalize
                   
                    // Move to next restriction
                    current_restriction++;
                   
                    // Clear buffers
                    memset(day_buffer, 0, sizeof(day_buffer));
                    memset(month_buffer, 0, sizeof(month_buffer));
                    memset(year_buffer, 0, sizeof(year_buffer));
                    entering_day = false;
                    entering_month = false;
                    entering_year = false;
                   
                    PlaySound(click_sound);
                   
                    // If all restrictions entered, update count
                    if (current_restriction >= restriction_count) {
                        num_restricted_days = restriction_count;
                    }
                } else {
                    show_notification("Invalid date");
                }
            } else {
                show_notification("Please enter all date fields");
            }
        }
    }
   
    if (current_restriction >= restriction_count || restriction_count == 0) {
        // "Continue" button when all restrictions entered or none needed
        if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 450, BUTTON_WIDTH, BUTTON_HEIGHT}, "Continue", 20)) {
            num_restricted_days = restriction_count;
            current_state = DATE_INPUT;
            PlaySound(click_sound);
        }
    }
   
    // Back button
    if (button_clicked((Rectangle){50, SCREEN_HEIGHT - 70, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", 20)) {
        current_restriction = 0;
        restriction_count = 0;
        memset(day_buffer, 0, sizeof(day_buffer));
        memset(month_buffer, 0, sizeof(month_buffer));
        memset(year_buffer, 0, sizeof(year_buffer));
        entering_day = false;
        entering_month = false;
        entering_year = false;
        current_state = TEAM_INPUT;
        PlaySound(click_sound);
    }
}

void input_dates_screen(void) {
    static bool entering_start_day = false;
    static bool entering_start_month = false;
    static bool entering_start_year = false;
    static bool entering_end_day = false;
    static bool entering_end_month = false;
    static bool entering_end_year = false;
   
    // Draw title
    DrawText("Tournament Dates", SCREEN_WIDTH/2 - MeasureText("Tournament Dates", 30)/2, 50, 30, WHITE);
   
    // Start date inputs
    DrawText("Start Date", 400, 120, 24, WHITE);
   
    // Start day input
    DrawText("Day:", 400, 170, 20, WHITE);
    Rectangle start_day_rect = {400, 200, 80, 40};
    DrawRectangleRec(start_day_rect, entering_start_day ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(start_day_rect, 2, DARKBLUE);
    DrawText(start_day_buffer, start_day_rect.x + 10, start_day_rect.y + 10, 20, BLACK);
   
    if (CheckCollisionPointRec(GetMousePosition(), start_day_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        entering_start_day = true;
        entering_start_month = false;
        entering_start_year = false;
        entering_end_day = false;
        entering_end_month = false;
        entering_end_year = false;
    }
   
   // Start month input
    DrawText("Month:", 500, 170, 20, WHITE);
    Rectangle start_month_rect = {500, 200, 80, 40};
    DrawRectangleRec(start_month_rect, entering_start_month ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(start_month_rect, 2, DARKBLUE);
    DrawText(start_month_buffer, start_month_rect.x + 10, start_month_rect.y + 10, 20, BLACK);
   
    if (CheckCollisionPointRec(GetMousePosition(), start_month_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        entering_start_day = false;
        entering_start_month = true;
        entering_start_year = false;
        entering_end_day = false;
        entering_end_month = false;
        entering_end_year = false;
    }
   
    // Start year input
    DrawText("Year:", 600, 170, 20, WHITE);
    Rectangle start_year_rect = {600, 200, 100, 40};
    DrawRectangleRec(start_year_rect, entering_start_year ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(start_year_rect, 2, DARKBLUE);
    DrawText(start_year_buffer, start_year_rect.x + 10, start_year_rect.y + 10, 20, BLACK);
   
    if (CheckCollisionPointRec(GetMousePosition(), start_year_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        entering_start_day = false;
        entering_start_month = false;
        entering_start_year = true;
        entering_end_day = false;
        entering_end_month = false;
        entering_end_year = false;
    }
   
    // End date inputs
    DrawText("End Date", 400, 270, 24, WHITE);
   
    // End day input
    DrawText("Day:", 400, 320, 20, WHITE);
    Rectangle end_day_rect = {400, 350, 80, 40};
    DrawRectangleRec(end_day_rect, entering_end_day ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(end_day_rect, 2, DARKBLUE);
    DrawText(end_day_buffer, end_day_rect.x + 10, end_day_rect.y + 10, 20, BLACK);
   
    if (CheckCollisionPointRec(GetMousePosition(), end_day_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        entering_start_day = false;
        entering_start_month = false;
        entering_start_year = false;
        entering_end_day = true;
        entering_end_month = false;
        entering_end_year = false;
    }
   
    // End month input
    DrawText("Month:", 500, 320, 20, WHITE);
    Rectangle end_month_rect = {500, 350, 80, 40};
    DrawRectangleRec(end_month_rect, entering_end_month ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(end_month_rect, 2, DARKBLUE);
    DrawText(end_month_buffer, end_month_rect.x + 10, end_month_rect.y + 10, 20, BLACK);
   
    if (CheckCollisionPointRec(GetMousePosition(), end_month_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        entering_start_day = false;
        entering_start_month = false;
        entering_start_year = false;
        entering_end_day = false;
        entering_end_month = true;
        entering_end_year = false;
    }
   
    // End year input
    DrawText("Year:", 600, 320, 20, WHITE);
    Rectangle end_year_rect = {600, 350, 100, 40};
    DrawRectangleRec(end_year_rect, entering_end_year ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(end_year_rect, 2, DARKBLUE);
    DrawText(end_year_buffer, end_year_rect.x + 10, end_year_rect.y + 10, 20, BLACK);
   
    if (CheckCollisionPointRec(GetMousePosition(), end_year_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        entering_start_day = false;
        entering_start_month = false;
        entering_start_year = false;
        entering_end_day = false;
        entering_end_month = false;
        entering_end_year = true;
    }
   
    // Handle text input for all fields
    if (entering_start_day) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= '0') && (key <= '9') && (strlen(start_day_buffer) < 2)) {
                start_day_buffer[strlen(start_day_buffer)] = (char)key;
                start_day_buffer[strlen(start_day_buffer)] = '\0';
            }
            key = GetCharPressed();
        }
       
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(start_day_buffer) > 0) {
            start_day_buffer[strlen(start_day_buffer) - 1] = '\0';
        }
    }
   
    if (entering_start_month) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= '0') && (key <= '9') && (strlen(start_month_buffer) < 2)) {
                start_month_buffer[strlen(start_month_buffer)] = (char)key;
                start_month_buffer[strlen(start_month_buffer)] = '\0';
            }
            key = GetCharPressed();
        }
       
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(start_month_buffer) > 0) {
            start_month_buffer[strlen(start_month_buffer) - 1] = '\0';
        }
    }
   
    if (entering_start_year) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= '0') && (key <= '9') && (strlen(start_year_buffer) < 4)) {
                start_year_buffer[strlen(start_year_buffer)] = (char)key;
                start_year_buffer[strlen(start_year_buffer)] = '\0';
            }
            key = GetCharPressed();
        }
       
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(start_year_buffer) > 0) {
            start_year_buffer[strlen(start_year_buffer) - 1] = '\0';
        }
    }
   
    if (entering_end_day) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= '0') && (key <= '9') && (strlen(end_day_buffer) < 2)) {
                end_day_buffer[strlen(end_day_buffer)] = (char)key;
                end_day_buffer[strlen(end_day_buffer)] = '\0';
            }
            key = GetCharPressed();
        }
       
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(end_day_buffer) > 0) {
            end_day_buffer[strlen(end_day_buffer) - 1] = '\0';
        }
    }
   
    if (entering_end_month) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= '0') && (key <= '9') && (strlen(end_month_buffer) < 2)) {
                end_month_buffer[strlen(end_month_buffer)] = (char)key;
                end_month_buffer[strlen(end_month_buffer)] = '\0';
            }
            key = GetCharPressed();
        }
       
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(end_month_buffer) > 0) {
            end_month_buffer[strlen(end_month_buffer) - 1] = '\0';
        }
    }
   
    if (entering_end_year) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= '0') && (key <= '9') && (strlen(end_year_buffer) < 4)) {
                end_year_buffer[strlen(end_year_buffer)] = (char)key;
                end_year_buffer[strlen(end_year_buffer)] = '\0';
            }
            key = GetCharPressed();
        }
       
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(end_year_buffer) > 0) {
            end_year_buffer[strlen(end_year_buffer) - 1] = '\0';
        }
    }
   
    // Generate Schedule button
    if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 450, BUTTON_WIDTH, BUTTON_HEIGHT}, "Generate Schedule", 20)) {
        // Validate input
        if (strlen(start_day_buffer) > 0 && strlen(start_month_buffer) > 0 && strlen(start_year_buffer) > 0 &&
            strlen(end_day_buffer) > 0 && strlen(end_month_buffer) > 0 && strlen(end_year_buffer) > 0) {
           
            int start_day = atoi(start_day_buffer);
            int start_month = atoi(start_month_buffer);
            int start_year = atoi(start_year_buffer);
           
            int end_day = atoi(end_day_buffer);
            int end_month = atoi(end_month_buffer);
            int end_year = atoi(end_year_buffer);
           
            // Basic validation
            if (start_day >= 1 && start_day <= 31 && start_month >= 1 && start_month <= 12 && start_year >= 2023 &&
                end_day >= 1 && end_day <= 31 && end_month >= 1 && end_month <= 12 && end_year >= 2023) {
               
                // Set tournament dates
                start_date.tm_mday = start_day;
                start_date.tm_mon = start_month - 1;
                start_date.tm_year = start_year - 1900;
                mktime(&start_date); // Normalize
               
                end_date.tm_mday = end_day;
                end_date.tm_mon = end_month - 1;
                end_date.tm_year = end_year - 1900;
                mktime(&end_date); // Normalize
               
                // Check if end date is after start date
                if (difftime(mktime(&end_date), mktime(&start_date)) > 0) {
                    // Generate schedule
                    generate_schedule_without_consecutive_matches();
                    cancel_matches_due_to_rain();
                    current_state = SCHEDULE_VIEW;
                    PlaySound(click_sound);
                } else {
                    show_notification("End date must be after start date");
                }
            } else {
                show_notification("Invalid date(s)");
            }
        } else {
            show_notification("Please fill in all date fields");
        }
    }
   
    // Back button
    if (button_clicked((Rectangle){50, SCREEN_HEIGHT - 70, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", 20)) {
        // Clear date buffers
        memset(start_day_buffer, 0, sizeof(start_day_buffer));
        memset(start_month_buffer, 0, sizeof(start_month_buffer));
        memset(start_year_buffer, 0, sizeof(start_year_buffer));
        memset(end_day_buffer, 0, sizeof(end_day_buffer));
        memset(end_month_buffer, 0, sizeof(end_month_buffer));
        memset(end_year_buffer, 0, sizeof(end_year_buffer));
        entering_start_day = false;
        entering_start_month = false;
        entering_start_year = false;
        entering_end_day = false;
        entering_end_month = false;
        entering_end_year = false;
        current_state = RESTRICTION_INPUT;
        PlaySound(click_sound);
    }
}

void schedule_view_screen(void) {
    // Draw title
    DrawText("Match Schedule", SCREEN_WIDTH/2 - MeasureText("Match Schedule", 30)/2, 50, 30, WHITE);
   
    // Create scrolling area for matches
    int y_pos = 100 - scroll_position;
    int items_visible = 0;
   
    for (int i = 0; i < num_matches; i++) {
        if (y_pos >= 100 && y_pos <= SCREEN_HEIGHT - 200) {
            items_visible++;
           
            Match *m = &matches[i];
           
            // Background rectangle for match info
            Color match_bg = ColorAlpha(BLACK, 0.7f);
            if (m->is_cancelled) {
                match_bg = ColorAlpha(MAROON, 0.7f);
            }
           
            DrawRectangle(200, y_pos, SCREEN_WIDTH - 400, 80, match_bg);
           
            // Match date and stadium
            DrawText(TextFormat("%02d-%02d-%04d at %s",
                             m->match_date.tm_mday,
                             m->match_date.tm_mon + 1,
                             m->match_date.tm_year + 1900,
                             m->stadium),
                  220, y_pos + 10, 16, WHITE);
           
            // Team names with team colors
            DrawText(teams[m->home_index].name, 220, y_pos + 35, 20, teams[m->home_index].color);
            DrawText("vs", 450, y_pos + 35, 20, WHITE);
            DrawText(teams[m->away_index].name, 500, y_pos + 35, 20, teams[m->away_index].color);
           
            // Score
            if (m->is_cancelled) {
                DrawText("MATCH CANCELLED (RAIN)", 700, y_pos + 35, 20, RED);
            } else {
                DrawText(TextFormat("%d - %d", m->home_runs, m->away_runs), 750, y_pos + 35, 20, WHITE);
               
                // Show winner
                if (m->home_runs > m->away_runs) {
                    DrawText("WINNER", 850, y_pos + 35, 16, teams[m->home_index].color);
                } else if (m->away_runs > m->home_runs) {
                    DrawText("WINNER", 850, y_pos + 35, 16, teams[m->away_index].color);
                } else {
                    DrawText("TIED", 850, y_pos + 35, 16, GOLD);
                }
            }
        }
       
        y_pos += 90; // Space between match entries
    }
   
    // Navigation controls
    DrawText("Use mouse wheel to scroll", SCREEN_WIDTH/2 - MeasureText("Use mouse wheel to scroll", 20)/2, SCREEN_HEIGHT - 150, 20, WHITE);
   
    // Points Table button
    if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, SCREEN_HEIGHT - 100, BUTTON_WIDTH, BUTTON_HEIGHT}, "Points Table", 20)) {
        display_points_table(); // Update points table
        current_state = POINTS_VIEW;
        PlaySound(click_sound);
    }
   
    // Back button
    if (button_clicked((Rectangle){50, SCREEN_HEIGHT - 70, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", 20)) {
        current_state = DATE_INPUT;
        PlaySound(click_sound);
    }
}

void points_view_screen(void) {
    // Draw title
    DrawText("Points Table", SCREEN_WIDTH/2 - MeasureText("Points Table", 30)/2, 50, 30, WHITE);
   
    // Draw table headers
    DrawRectangle(150, 100, SCREEN_WIDTH - 300, 40, ColorAlpha(BLACK, 0.8f));
    DrawText("Team", 170, 110, 20, WHITE);
    DrawText("Points", 400, 110, 20, WHITE);
    DrawText("Won", 500, 110, 20, WHITE);
    DrawText("Lost", 580, 110, 20, WHITE);
    DrawText("Played", 660, 110, 20, WHITE);
    DrawText("NRR", 760, 110, 20, WHITE);
   
    // Draw team rows
    for (int i = 0; i < num_teams; i++) {
        // Alternate row colors
        DrawRectangle(150, 140 + i * 40, SCREEN_WIDTH - 300, 40,
                    i % 2 == 0 ? ColorAlpha(DARKBLUE, 0.5f) : ColorAlpha(DARKBLUE, 0.3f));
       
        // Team name with team color
        DrawText(teams[i].name, 170, 150 + i * 40, 20, teams[i].color);
       
        // Team stats
        DrawText(TextFormat("%d", teams[i].points), 400, 150 + i * 40, 20, WHITE);
        DrawText(TextFormat("%d", teams[i].matches_won), 500, 150 + i * 40, 20, WHITE);
        DrawText(TextFormat("%d", teams[i].matches_lost), 580, 150 + i * 40, 20, WHITE);
        DrawText(TextFormat("%d", teams[i].matches_played), 660, 150 + i * 40, 20, WHITE);
        DrawText(TextFormat("%.2f", teams[i].net_run_rate), 760, 150 + i * 40, 20, WHITE);
    }
   
    // Playoffs button
    if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, SCREEN_HEIGHT - 100, BUTTON_WIDTH, BUTTON_HEIGHT}, "Playoffs", 20)) {
        current_state = PLAYOFF_INPUT;
        playoffs_stage = 0;
        PlaySound(click_sound);
    }
   
    // Back button
    if (button_clicked((Rectangle){50, SCREEN_HEIGHT - 70, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", 20)) {
        current_state = SCHEDULE_VIEW;
        PlaySound(click_sound);
    }
}

void playoff_input_screen(void) {
    static int current_home_runs = 0;
    static int current_away_runs = 0;
    static bool entering_home_runs = false;
    static bool entering_away_runs = false;
    static char home_runs_buffer[4] = {0};
    static char away_runs_buffer[4] = {0};
   
    // Draw background for playoffs with trophy silhouette
    DrawTexture(trophy_texture, SCREEN_WIDTH - 300, 100, ColorAlpha(WHITE, 0.3f));
   
    // Draw title
    DrawText("IPL Playoffs", SCREEN_WIDTH/2 - MeasureText("IPL Playoffs", 30)/2, 50, 30, GOLD);
   
    switch (playoffs_stage) {
        case 0: {
            // Qualifier 1: 1st vs 2nd
            DrawText("Qualifier 1", SCREEN_WIDTH/2 - MeasureText("Qualifier 1", 28)/2, 100, 28, WHITE);
            DrawText(TextFormat("%s vs %s", teams[0].name, teams[1].name),
                    SCREEN_WIDTH/2 - MeasureText(TextFormat("%s vs %s", teams[0].name, teams[1].name), 24)/2,
                    140, 24, WHITE);
           
            // Draw team names with team colors
            DrawText(teams[0].name, 300, 200, 24, teams[0].color);
            DrawText("vs", SCREEN_WIDTH/2 - MeasureText("vs", 24)/2, 200, 24, WHITE);
            DrawText(teams[1].name, 700, 200, 24, teams[1].color);
           
            // Input fields for runs
            DrawText("Runs:", 300, 250, 20, WHITE);
            Rectangle home_runs_rect = {300, 280, 100, 40};
            DrawRectangleRec(home_runs_rect, entering_home_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(home_runs_rect, 2, teams[0].color);
            DrawText(home_runs_buffer, home_runs_rect.x + 10, home_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), home_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = true;
                entering_away_runs = false;
            }
           
            DrawText("Runs:", 700, 250, 20, WHITE);
            Rectangle away_runs_rect = {700, 280, 100, 40};
            DrawRectangleRec(away_runs_rect, entering_away_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(away_runs_rect, 2, teams[1].color);
            DrawText(away_runs_buffer, away_runs_rect.x + 10, away_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), away_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = false;
                entering_away_runs = true;
            }
           
            // Handle text input
            if (entering_home_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(home_runs_buffer) < 3)) {
                        home_runs_buffer[strlen(home_runs_buffer)] = (char)key;
                        home_runs_buffer[strlen(home_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(home_runs_buffer) > 0) {
                    home_runs_buffer[strlen(home_runs_buffer) - 1] = '\0';
                }
            }
           
            if (entering_away_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(away_runs_buffer) < 3)) {
                        away_runs_buffer[strlen(away_runs_buffer)] = (char)key;
                        away_runs_buffer[strlen(away_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(away_runs_buffer) > 0) {
                    away_runs_buffer[strlen(away_runs_buffer) - 1] = '\0';
                }
            }
           
            // Submit button
            if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 350, BUTTON_WIDTH, BUTTON_HEIGHT}, "Submit Result", 20)) {
                if (strlen(home_runs_buffer) > 0 && strlen(away_runs_buffer) > 0) {
                    q1_home = atoi(home_runs_buffer);
                    q1_away = atoi(away_runs_buffer);
                   
                    // Handle super over if tied
                    if (q1_home == q1_away) {
                        DrawText("Match tied! Super Over required.",
                                SCREEN_WIDTH/2 - MeasureText("Match tied! Super Over required.", 20)/2,
                                400, 20, GOLD);
                        show_notification("Match tied! Enter Super Over scores");
                    } else {
                        // Determine winner
                        q1_winner = q1_home > q1_away ? 0 : 1;
                        q1_loser = q1_home > q1_away ? 1 : 0;
                       
                        // Clear buffers and move to next stage
                        memset(home_runs_buffer, 0, sizeof(home_runs_buffer));
                        memset(away_runs_buffer, 0, sizeof(away_runs_buffer));
                        entering_home_runs = false;
                        entering_away_runs = false;
                        playoffs_stage = 1;
                    }
                   
                    PlaySound(click_sound);
                } else {
                    show_notification("Please enter scores for both teams");
                }
            }
            break;
        }
       
        case 1: {
            // Eliminator: 3rd vs 4th
            DrawText("Eliminator", SCREEN_WIDTH/2 - MeasureText("Eliminator", 28)/2, 100, 28, WHITE);
            DrawText(TextFormat("%s vs %s", teams[2].name, teams[3].name),
                    SCREEN_WIDTH/2 - MeasureText(TextFormat("%s vs %s", teams[2].name, teams[3].name), 24)/2,
                    140, 24, WHITE);
           
            // Draw team names with team colors
            DrawText(teams[2].name, 300, 200, 24, teams[2].color);
            DrawText("vs", SCREEN_WIDTH/2 - MeasureText("vs", 24)/2, 200, 24, WHITE);
            DrawText(teams[3].name, 700, 200, 24, teams[3].color);
           
            // Input fields for runs
            DrawText("Runs:", 300, 250, 20, WHITE);
            Rectangle home_runs_rect = {300, 280, 100, 40};
            DrawRectangleRec(home_runs_rect, entering_home_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(home_runs_rect, 2, teams[2].color);
            DrawText(home_runs_buffer, home_runs_rect.x + 10, home_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), home_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = true;
                entering_away_runs = false;
            }
           
            DrawText("Runs:", 700, 250, 20, WHITE);
            Rectangle away_runs_rect = {700, 280, 100, 40};
            DrawRectangleRec(away_runs_rect, entering_away_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(away_runs_rect, 2, teams[3].color);
            DrawText(away_runs_buffer, away_runs_rect.x + 10, away_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), away_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = false;
                entering_away_runs = true;
            }
           
            // Handle text input
            if (entering_home_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(home_runs_buffer) < 3)) {
                        home_runs_buffer[strlen(home_runs_buffer)] = (char)key;
                        home_runs_buffer[strlen(home_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(home_runs_buffer) > 0) {
                    home_runs_buffer[strlen(home_runs_buffer) - 1] = '\0';
                }
            }
           
            if (entering_away_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(away_runs_buffer) < 3)) {
                        away_runs_buffer[strlen(away_runs_buffer)] = (char)key;
                        away_runs_buffer[strlen(away_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(away_runs_buffer) > 0) {
                    away_runs_buffer[strlen(away_runs_buffer) - 1] = '\0';
                }
            }
           
            // Submit button
            if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 350, BUTTON_WIDTH, BUTTON_HEIGHT}, "Submit Result", 20)) {
                if (strlen(home_runs_buffer) > 0 && strlen(away_runs_buffer) > 0) {
                    elim_home = atoi(home_runs_buffer);
                    elim_away = atoi(away_runs_buffer);
                   
                    // Handle super over if tied
                    if (elim_home == elim_away) {
                        DrawText("Match tied! Super Over required.",
                                SCREEN_WIDTH/2 - MeasureText("Match tied! Super Over required.", 20)/2,
                                400, 20, GOLD);
                        show_notification("Match tied! Enter Super Over scores");
                    } else {
                        // Determine winner
                        elim_winner = elim_home > elim_away ? 2 : 3;
                       
                        // Clear buffers and move to next stage
                        memset(home_runs_buffer, 0, sizeof(home_runs_buffer));
                        memset(away_runs_buffer, 0, sizeof(away_runs_buffer));
                        entering_home_runs = false;
                        entering_away_runs = false;
                        playoffs_stage = 2;
                    }
                   
                    PlaySound(click_sound);
                } else {
                    show_notification("Please enter scores for both teams");
                }
            }
            break;
        }
       
        case 2: {
            // Qualifier 2: Eliminator winner vs Qualifier 1 loser
            DrawText("Qualifier 2", SCREEN_WIDTH/2 - MeasureText("Qualifier 2", 28)/2, 100, 28, WHITE);
            DrawText(TextFormat("%s vs %s", teams[elim_winner].name, teams[q1_loser].name),
                    SCREEN_WIDTH/2 - MeasureText(TextFormat("%s vs %s", teams[elim_winner].name, teams[q1_loser].name), 24)/2,
                    140, 24, WHITE);
           
            // Draw team names with team colors
            DrawText(teams[elim_winner].name, 300, 200, 24, teams[elim_winner].color);
            DrawText("vs", SCREEN_WIDTH/2 - MeasureText("vs", 24)/2, 200, 24, WHITE);
            DrawText(teams[q1_loser].name, 700, 200, 24, teams[q1_loser].color);
           
            // Input fields for runs
            DrawText("Runs:", 300, 250, 20, WHITE);
            Rectangle home_runs_rect = {300, 280, 100, 40};
            DrawRectangleRec(home_runs_rect, entering_home_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(home_runs_rect, 2, teams[elim_winner].color);
            DrawText(home_runs_buffer, home_runs_rect.x + 10, home_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), home_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = true;
                entering_away_runs = false;
            }
           
            DrawText("Runs:", 700, 250, 20, WHITE);
            Rectangle away_runs_rect = {700, 280, 100, 40};
            DrawRectangleRec(away_runs_rect, entering_away_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(away_runs_rect, 2, teams[q1_loser].color);
            DrawText(away_runs_buffer, away_runs_rect.x + 10, away_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), away_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = false;
                entering_away_runs = true;
            }
           
            // Handle text input
            if (entering_home_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(home_runs_buffer) < 3)) {
                        home_runs_buffer[strlen(home_runs_buffer)] = (char)key;
                        home_runs_buffer[strlen(home_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(home_runs_buffer) > 0) {
                    home_runs_buffer[strlen(home_runs_buffer) - 1] = '\0';
                }
            }
           
            if (entering_away_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(away_runs_buffer) < 3)) {
                        away_runs_buffer[strlen(away_runs_buffer)] = (char)key;
                        away_runs_buffer[strlen(away_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(away_runs_buffer) > 0) {
                    away_runs_buffer[strlen(away_runs_buffer) - 1] = '\0';
                }
            }
           
            // Submit button
            if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 350, BUTTON_WIDTH, BUTTON_HEIGHT}, "Submit Result", 20)) {
                if (strlen(home_runs_buffer) > 0 && strlen(away_runs_buffer) > 0) {
                    q2_home = atoi(home_runs_buffer);
                    q2_away = atoi(away_runs_buffer);
                   
                    // Handle super over if tied
                    if (q2_home == q2_away) {
                        DrawText("Match tied! Super Over required.",
                                SCREEN_WIDTH/2 - MeasureText("Match tied! Super Over required.", 20)/2,
                                400, 20, GOLD);
                        show_notification("Match tied! Enter Super Over scores");
                    } else {
                        // Determine winner
                        q2_winner = q2_home > q2_away ? elim_winner : q1_loser;
                       
                        // Clear buffers and move to next stage
                        memset(home_runs_buffer, 0, sizeof(home_runs_buffer));
                        memset(away_runs_buffer, 0, sizeof(away_runs_buffer));
                        entering_home_runs = false;
                        entering_away_runs = false;
                        playoffs_stage = 3;
                    }
                   
                    PlaySound(click_sound);
                } else {
                    show_notification("Please enter scores for both teams");
                }
            }
            break;
        }
       
        case 3: {
            // Final: Q1 winner vs Q2 winner
            DrawText("IPL FINAL", SCREEN_WIDTH/2 - MeasureText("IPL FINAL", 36)/2, 100, 36, GOLD);
            DrawText(TextFormat("%s vs %s", teams[q1_winner].name, teams[q2_winner].name),
                    SCREEN_WIDTH/2 - MeasureText(TextFormat("%s vs %s", teams[q1_winner].name, teams[q2_winner].name), 28)/2,
                    150, 28, WHITE);
           
            // Draw team names with team colors
            DrawText(teams[q1_winner].name, 300, 200, 24, teams[q1_winner].color);
            DrawText("vs", SCREEN_WIDTH/2 - MeasureText("vs", 24)/2, 200, 24, WHITE);
            DrawText(teams[q2_winner].name, 700, 200, 24, teams[q2_winner].color);
           
            // Input fields for runs
            DrawText("Runs:", 300, 250, 20, WHITE);
            Rectangle home_runs_rect = {300, 280, 100, 40};
            DrawRectangleRec(home_runs_rect, entering_home_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(home_runs_rect, 2, teams[q1_winner].color);
            DrawText(home_runs_buffer, home_runs_rect.x + 10, home_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), home_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = true;
                entering_away_runs = false;
            }
           
            DrawText("Runs:", 700, 250, 20, WHITE);
            Rectangle away_runs_rect = {700, 280, 100, 40};
            DrawRectangleRec(away_runs_rect, entering_away_runs ? LIGHTGRAY : WHITE);
            DrawRectangleLinesEx(away_runs_rect, 2, teams[q2_winner].color);
            DrawText(away_runs_buffer, away_runs_rect.x + 10, away_runs_rect.y + 10, 20, BLACK);
           
            if (CheckCollisionPointRec(GetMousePosition(), away_runs_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                entering_home_runs = false;
                entering_away_runs = true;
            }
           
            // Handle text input
            if (entering_home_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(home_runs_buffer) < 3)) {
                        home_runs_buffer[strlen(home_runs_buffer)] = (char)key;
                        home_runs_buffer[strlen(home_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(home_runs_buffer) > 0) {
                    home_runs_buffer[strlen(home_runs_buffer) - 1] = '\0';
                }
            }
           
            if (entering_away_runs) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0') && (key <= '9') && (strlen(away_runs_buffer) < 3)) {
                        away_runs_buffer[strlen(away_runs_buffer)] = (char)key;
                        away_runs_buffer[strlen(away_runs_buffer)] = '\0';
                    }
                    key = GetCharPressed();
                }
               
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(away_runs_buffer) > 0) {
                    away_runs_buffer[strlen(away_runs_buffer) - 1] = '\0';
                }
            }
           
            // Submit button
            if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 350, BUTTON_WIDTH, BUTTON_HEIGHT}, "Submit Result", 20)) {
                if (strlen(home_runs_buffer) > 0 && strlen(away_runs_buffer) > 0) {
                    final_home = atoi(home_runs_buffer);
                    final_away = atoi(away_runs_buffer);
                   
                    // Handle super over if tied
                    if (final_home == final_away) {
                        DrawText("Match tied! Super Over required.",
                                SCREEN_WIDTH/2 - MeasureText("Match tied! Super Over required.", 20)/2,
                                400, 20, GOLD);
                        show_notification("Match tied! Enter Super Over scores");
                    } else {
                        // Determine winner
                        final_winner = final_home > final_away ? q1_winner : q2_winner;
                       
                        // Clear buffers and move to results stage
                        memset(home_runs_buffer, 0, sizeof(home_runs_buffer));
                        memset(away_runs_buffer, 0, sizeof(away_runs_buffer));
                        entering_home_runs = false;
                        entering_away_runs = false;
                        playoffs_stage = 4;
                    }
                   
                    PlaySound(click_sound);
                } else {
                    show_notification("Please enter scores for both teams");
                }
            }
            break;
        }
       
        case 4: {
            // Final results and champion display
            DrawText("IPL CHAMPION", SCREEN_WIDTH/2 - MeasureText("IPL CHAMPION", 36)/2, 100, 36, GOLD);
           
            // Draw trophy
            DrawTexture(trophy_texture, SCREEN_WIDTH/2 - trophy_texture.width/2, 150, WHITE);
           
            // Draw champion name with team color
            DrawText(teams[final_winner].name,
                    SCREEN_WIDTH/2 - MeasureText(teams[final_winner].name, 40)/2,
                    400, 40, teams[final_winner].color);
           
            // Draw congratulatory message
            DrawText("CONGRATULATIONS!",
                    SCREEN_WIDTH/2 - MeasureText("CONGRATULATIONS!", 30)/2,
                    460, 30, GOLD);
           
            // Draw the champion team's stats
            DrawText(TextFormat("Matches Played: %d", teams[final_winner].matches_played),
                    SCREEN_WIDTH/2 - 150, 520, 20, WHITE);
            DrawText(TextFormat("Matches Won: %d", teams[final_winner].matches_won),
                    SCREEN_WIDTH/2 - 150, 550, 20, WHITE);
            DrawText(TextFormat("Net Run Rate: %.2f", teams[final_winner].net_run_rate),
                    SCREEN_WIDTH/2 - 150, 580, 20, WHITE);
           
            // Restart button
            if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 650, BUTTON_WIDTH, BUTTON_HEIGHT}, "Show Awards", 20)) {
                current_state = AWARDS_VIEW;
                PlaySound(click_sound);
            }
            break;
        }
    }
   
    // Back button for all stages except final results
    if (playoffs_stage < 4) {
        if (button_clicked((Rectangle){50, SCREEN_HEIGHT - 70, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back", 20)) {
            current_state = POINTS_VIEW;
            PlaySound(click_sound);
        }
    }
}

void show_notification(const char *message) {
    notification_timer = 180; // 3 seconds at 60 fps
    strcpy(notification_message, message);
}

void draw_notification(void) {
    if (notification_timer > 0) {
        notification_timer--;
       
        int width = MeasureText(notification_message, 20) + 40;
        int height = 40;
       
        DrawRectangle(SCREEN_WIDTH/2 - width/2, SCREEN_HEIGHT - height - 20, width, height, ColorAlpha(BLACK, 0.8f));
        DrawRectangleLinesEx((Rectangle){SCREEN_WIDTH/2 - width/2, SCREEN_HEIGHT - height - 20, width, height}, 2, WHITE);
        DrawText(notification_message, SCREEN_WIDTH/2 - MeasureText(notification_message, 20)/2, SCREEN_HEIGHT - height - 10, 20, WHITE);
    }
}

void generate_schedule_without_consecutive_matches(void) {
    // Clear existing matches
    num_matches = 0;

    struct tm current_date = start_date;
    int team_last_played[MAX_TEAMS] = {-1}; // Tracks the last day of the year (tm_yday) each team played

    // Array to hold randomized team indices
    int team_indices[MAX_TEAMS];
    for (int i = 0; i < num_teams; i++) {
        team_indices[i] = i;
    }

    // Each day in the range
    while (difftime(mktime(&current_date), mktime(&end_date)) <= 0) {
        bool match_scheduled = false;

        // Shuffle the team indices to create randomness
        for (int i = num_teams - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = team_indices[i];
            team_indices[i] = team_indices[j];
            team_indices[j] = temp;
        }

        // Iterate through the shuffled team indices
        for (int x = 0; x < num_teams && !match_scheduled; x++) {
            for (int y = x + 1; y < num_teams && !match_scheduled; y++) {
                int home_index = team_indices[x];
                int away_index = team_indices[y];

                // Ensure neither team has played the previous day
                if (team_last_played[home_index] == current_date.tm_yday - 1 ||
                    team_last_played[away_index] == current_date.tm_yday - 1) {
                    continue; // Skip this pair to avoid consecutive matches
                }

                // Create a new match
                Match new_match;
                new_match.home_index = home_index;
                new_match.away_index = away_index;
                new_match.match_date = current_date;
                new_match.is_cancelled = false;

                // Assign stadium based on the home team
                strcpy(new_match.stadium, teams[new_match.home_index].home_stadium);

                // Generate random scores
                new_match.home_runs = GetRandomValue(120, 220);
                new_match.away_runs = GetRandomValue(120, 220);

                // Update team statistics
                teams[new_match.home_index].matches_played++;
                teams[new_match.away_index].matches_played++;

                if (new_match.home_runs > new_match.away_runs) {
                    teams[new_match.home_index].matches_won++;
                    teams[new_match.home_index].points += 2;
                    teams[new_match.away_index].matches_lost++;
                } else {
                    teams[new_match.away_index].matches_won++;
                    teams[new_match.away_index].points += 2;
                    teams[new_match.home_index].matches_lost++;
                }

                // Calculate net run rate
                teams[new_match.home_index].net_run_rate +=
                    ((float)new_match.home_runs - (float)new_match.away_runs) / 20.0f;
                teams[new_match.away_index].net_run_rate +=
                    ((float)new_match.away_runs - (float)new_match.home_runs) / 20.0f;

                // Add match to the schedule
                matches[num_matches++] = new_match;

                // Update the last match day for both teams
                team_last_played[home_index] = current_date.tm_yday;
                team_last_played[away_index] = current_date.tm_yday;

                // Mark a match as scheduled for this day
                match_scheduled = true;
            }
        }

        // Advance the date to the next day
        current_date.tm_mday++;
        mktime(&current_date); // Normalize date
    }
}




void cancel_matches_due_to_rain(void) {
    // Randomly cancel some matches due to rain (about 10-15%)
    for (int i = 0; i < num_matches; i++) {
        if (GetRandomValue(1, 100) <= 12) { // 12% chance of rain
            matches[i].is_cancelled = true;
        }
    }
}

void display_points_table(void) {
    // Sort teams by points (descending)
    for (int i = 0; i < num_teams - 1; i++) {
        for (int j = 0; j < num_teams - i - 1; j++) {
            if (teams[j].points < teams[j+1].points ||
                (teams[j].points == teams[j+1].points && teams[j].net_run_rate < teams[j+1].net_run_rate)) {
                // Swap teams
                Team temp = teams[j];
                teams[j] = teams[j+1];
                teams[j+1] = temp;
            }
        }
    }
}

void initialize_game(void) {
    num_teams = 0;
    num_matches = 0;
    playoffs_stage = 0;
    q1_winner = -1;
    q1_loser = -1;
    elim_winner = -1;
    q2_winner = -1;
    final_winner = -1;
    awards_generated = false;

    memset(start_day_buffer, 0, sizeof(start_day_buffer));
    memset(start_month_buffer, 0, sizeof(start_month_buffer));
    memset(start_year_buffer, 0, sizeof(start_year_buffer));
    memset(end_day_buffer, 0, sizeof(end_day_buffer));
    memset(end_month_buffer, 0, sizeof(end_month_buffer));
    memset(end_year_buffer, 0, sizeof(end_year_buffer));

    notification_timer = 0;
    memset(notification_message, 0, sizeof(notification_message));

    scroll_position = 0;

    background_texture = LoadTexture("resources/ipl_background.png");
    ipl_logo = LoadTexture("resources/ipl_logo.png");
    trophy_texture = LoadTexture("resources/trophy.png");
    orange_cap_texture = LoadTexture("resources/orange_cap.png");
    purple_cap_texture = LoadTexture("resources/purple_cap.png");

    cricket_font = LoadFont("resources/cricket_font.ttf");

    background_music = LoadMusicStream("resources/ipl_theme.mp3");
    click_sound = LoadSound("resources/click.wav");

    SetMusicVolume(background_music, 0.5f);
    PlayMusicStream(background_music);

    for (int i = 0; i < MAX_TEAMS; i++) {
        team_last_match_day[i] = -2;
    }
}

// Updates the message of the day based on current tournament state
void update_message(void) {
    static char message_buffer[256] = {0};
    static int message_timer = 0;
    static int message_index = 0;
    
    // Array of possible messages
    const char* messages[] = {
        "Welcome to the IPL Tournament Manager!",
        "Did you know? The IPL was founded in 2008.",
        "The Mumbai Indians have won the most IPL titles.",
        "Chennai Super Kings has one of the highest win percentages in IPL history.",
        "IPL matches have a typical attendance of 30,000+ fans.",
        "The IPL season typically runs from March to May.",
        "The IPL introduced the strategic timeout in 2009.",
        "The highest team score in IPL history is 263/5 by RCB.",
        "Chris Gayle holds the record for most sixes in IPL history.",
        "The IPL uses the DRS (Decision Review System) for close calls."
    };
    const int num_messages = sizeof(messages) / sizeof(messages[0]);
    
    // Update message timer and change message if needed
    message_timer++;
    if (message_timer >= 600) { // Change message every 10 seconds (at 60 FPS)
        message_timer = 0;
        message_index = (message_index + 1) % num_messages;
        strcpy(message_buffer, messages[message_index]);
    }
    
    // Display the current tournament state if applicable
    if (current_state >= SCHEDULE_VIEW) {
        // Check how many matches have been played
        int completed_matches = 0;
        for (int i = 0; i < num_matches; i++) {
            if (!matches[i].is_cancelled) {
                completed_matches++;
            }
        }
        
        // Find the current leading team
        int leading_team = 0;
        for (int i = 1; i < num_teams; i++) {
            if (teams[i].points > teams[leading_team].points) {
                leading_team = i;
            }
            else if (teams[i].points == teams[leading_team].points && 
                     teams[i].net_run_rate > teams[leading_team].net_run_rate) {
                leading_team = i;
            }
        }
        
        // Set context-aware message
        if (playoffs_stage >= 1 && playoffs_stage <= 3) {
            sprintf(message_buffer, "Playoffs in progress! Watch the excitement unfold!");
        }
        else if (playoffs_stage == 4) {
            sprintf(message_buffer, "Congratulations to %s for winning the tournament!", teams[final_winner].name);
        }
        else if (completed_matches > 0) {
            sprintf(message_buffer, "%s is currently leading the points table with %d points!", 
                    teams[leading_team].name, teams[leading_team].points);
        }
    }
    
    // Draw the message at the bottom of the screen
    int text_width = MeasureText(message_buffer, 20);
    DrawRectangle(SCREEN_WIDTH/2 - text_width/2 - 20, SCREEN_HEIGHT - 40, text_width + 40, 30, ColorAlpha(BLACK, 0.7f));
    DrawText(message_buffer, SCREEN_WIDTH/2 - text_width/2, SCREEN_HEIGHT - 35, 20, WHITE);
}

// Awards view screen showing various tournament statistics and awards
void awards_view_screen(void) {
    // Find players with best statistics (these would be calculated from the player stats)
    static int highest_run_scorer = -1;
    static int most_wickets_taker = -1;
    static int highest_strike_rate = -1;
    static int best_economy_rate = -1;
    static int most_sixes_hitter = -1;
    static int best_fielder = -1;

    if (!awards_generated) {    
        // For demonstration, we'll determine these from our existing team data
        // In a real implementation, you'd have player stats to analyze
        highest_run_scorer = GetRandomValue(0, num_teams - 1);
        most_wickets_taker = GetRandomValue(0, num_teams - 1);
        highest_strike_rate = GetRandomValue(0, num_teams - 1);
        best_economy_rate = GetRandomValue(0, num_teams - 1);
        most_sixes_hitter = GetRandomValue(0, num_teams - 1);
        best_fielder = GetRandomValue(0, num_teams - 1);
        awards_generated = true;
    }
    
    // Draw title
    DrawText("IPL Tournament Awards", SCREEN_WIDTH/2 - MeasureText("IPL Tournament Awards", 32)/2, 50, 32, GOLD);
    
    // Draw trophy icon
    DrawTexture(trophy_texture, 50, 50, WHITE);
    
    // Orange Cap (Highest Run Scorer)
    DrawRectangle(200, 130, SCREEN_WIDTH - 400, 60, ColorAlpha(ORANGE, 0.7f));
    DrawText("ORANGE CAP", 220, 140, 24, WHITE);
    DrawText("Highest Run Scorer", 220, 170, 18, WHITE);
    DrawText(TextFormat("%s from %s", players[highest_run_scorer]->name, teams[players[highest_run_scorer]->team_index].name), 
             550, 150, 20, WHITE);
    DrawText(TextFormat("Runs: %d", players[highest_run_scorer]->runs), 850, 150, 20, WHITE);
    
    // Purple Cap (Most Wickets)
    DrawRectangle(200, 210, SCREEN_WIDTH - 400, 60, ColorAlpha(PURPLE, 0.7f));
    DrawText("PURPLE CAP", 220, 220, 24, WHITE);
    DrawText("Most Wickets", 220, 250, 18, WHITE);
    DrawText(TextFormat("%s from %s", players[most_wickets_taker]->name, teams[players[most_wickets_taker]->team_index].name), 
             550, 230, 20, WHITE);
    DrawText(TextFormat("Wickets: %d", players[most_wickets_taker]->wickets), 850, 230, 20, WHITE);
    
    // Maximum Sixes Award
    DrawRectangle(200, 290, SCREEN_WIDTH - 400, 60, ColorAlpha(SKYBLUE, 0.7f));
    DrawText("MAXIMUM SIXES", 220, 300, 24, WHITE);
    DrawText("Most Sixes Hit", 220, 330, 18, WHITE);
    DrawText(TextFormat("%s from %s", players[most_sixes_hitter]->name, teams[players[most_sixes_hitter]->team_index].name), 
             550, 310, 20, WHITE);
    DrawText(TextFormat("Sixes: %d", players[most_sixes_hitter]->sixes), 850, 310, 20, WHITE);
    
    // Strike Rate Award
    DrawRectangle(200, 370, SCREEN_WIDTH - 400, 60, ColorAlpha(LIME, 0.7f));
    DrawText("STRIKER AWARD", 220, 380, 24, WHITE);
    DrawText("Highest Strike Rate", 220, 410, 18, WHITE);
    DrawText(TextFormat("%s from %s", players[highest_strike_rate]->name, teams[players[highest_strike_rate]->team_index].name), 
             550, 390, 20, WHITE);
    DrawText(TextFormat("Strike Rate: %.2f", players[highest_strike_rate]->strike_rate), 850, 390, 20, WHITE);
    
    // Economy Rate Award
    DrawRectangle(200, 450, SCREEN_WIDTH - 400, 60, ColorAlpha(PINK, 0.7f));
    DrawText("ECONOMY AWARD", 220, 460, 24, WHITE);
    DrawText("Best Economy Rate", 220, 490, 18, WHITE);
    DrawText(TextFormat("%s from %s", players[best_economy_rate]->name, teams[players[best_economy_rate]->team_index].name), 
             550, 470, 20, WHITE);
    DrawText(TextFormat("Economy: %.2f", players[best_economy_rate]->economy), 850, 470, 20, WHITE);
    
    // Fielding Award
    DrawRectangle(200, 530, SCREEN_WIDTH - 400, 60, ColorAlpha(YELLOW, 0.7f));
    DrawText("GOLDEN GLOVE", 220, 540, 24, WHITE);
    DrawText("Best Fielder", 220, 570, 18, WHITE);
    DrawText(TextFormat("%s from %s", players[best_fielder]->name, teams[players[best_fielder]->team_index].name), 
             550, 550, 20, WHITE);
    DrawText(TextFormat("Catches: %d, Run Outs: %d", players[best_fielder]->catches, players[best_fielder]->run_outs), 
             750, 550, 20, WHITE);
    
    // Navigation buttons
    if (button_clicked((Rectangle){SCREEN_WIDTH/2 - BUTTON_WIDTH/2, 620, BUTTON_WIDTH, BUTTON_HEIGHT}, "Back to Points", 20)) {
        current_state = POINTS_VIEW;
        PlaySound(click_sound);
    }
    
    if (button_clicked((Rectangle){50, SCREEN_HEIGHT - 70, BUTTON_WIDTH, BUTTON_HEIGHT}, "Main Menu", 20)) {
        initialize_game();
        current_state = MENU;
        PlaySound(click_sound);
    }
}

// Generate random players for all teams with realistic cricket statistics
void generate_players(void) {
    // Player names (first names and last names to mix and match)
    const char* first_names[] = {
        "Virat", "Rohit", "MS", "Ravindra", "Jasprit", "KL", "Hardik", "Shikhar", "Rishabh", 
        "Surya", "Shreyas", "Yuzvendra", "Mohammed", "Ravichandran", "Ajinkya", "Dinesh", 
        "Bhuvneshwar", "Kuldeep", "Axar", "Shardul", "Ishan", "Washington", "Deepak", 
        "Umesh", "Mayank", "Prithvi", "Sanju", "Devdutt", "Ruturaj", "Rahul"
    };
    
    const char* last_names[] = {
        "Kohli", "Sharma", "Dhoni", "Jadeja", "Bumrah", "Rahul", "Pandya", "Dhawan", "Pant", 
        "Kumar", "Iyer", "Chahal", "Shami", "Ashwin", "Rahane", "Karthik", "Kumar", "Yadav", 
        "Patel", "Thakur", "Kishan", "Sundar", "Chahar", "Yadav", "Agarwal", "Shaw", "Samson", 
        "Padikkal", "Gaikwad", "Tewatia", "Gill", "Singh", "Khan", "Malik", "Arshdeep"
    };
    
    const int num_first_names = sizeof(first_names) / sizeof(first_names[0]);
    const int num_last_names = sizeof(last_names) / sizeof(last_names[0]);
    
    // Player roles
    const char* roles[] = {
        "Batsman", "Bowler", "All-rounder", "Wicket-keeper"
    };
    
    // Clear existing players
    num_players = 0;
    
    // Generate 11 players for each team
    for (int team_idx = 0; team_idx < num_teams; team_idx++) {
        for (int p = 0; p < 11; p++) {
            Player new_player;
            
            // Assign player to team
            new_player.team_index = team_idx;
            
            // Generate random name
            sprintf(new_player.name, "%s %s", 
                    first_names[GetRandomValue(0, num_first_names - 1)], 
                    last_names[GetRandomValue(0, num_last_names - 1)]);
            
            // Assign role
            int role_idx = GetRandomValue(0, 3);
            strcpy(new_player.role, roles[role_idx]);
            
            // Generate statistics based on role
            switch (role_idx) {
                case 0: // Batsman
                    new_player.runs = GetRandomValue(200, 600);
                    new_player.wickets = GetRandomValue(0, 3);
                    new_player.strike_rate = 115.0f + ((float)GetRandomValue(0, 500) / 10.0f);
                    new_player.economy = 8.0f + ((float)GetRandomValue(-20, 40) / 10.0f);
                    new_player.sixes = GetRandomValue(5, 30);
                    new_player.fours = GetRandomValue(15, 70);
                    new_player.catches = GetRandomValue(1, 8);
                    new_player.run_outs = GetRandomValue(0, 3);
                    break;
                    
                case 1: // Bowler
                    new_player.runs = GetRandomValue(50, 150);
                    new_player.wickets = GetRandomValue(10, 25);
                    new_player.strike_rate = 90.0f + ((float)GetRandomValue(0, 300) / 10.0f);
                    new_player.economy = 6.5f + ((float)GetRandomValue(-20, 35) / 10.0f);
                    new_player.sixes = GetRandomValue(0, 5);
                    new_player.fours = GetRandomValue(3, 15);
                    new_player.catches = GetRandomValue(3, 12);
                    new_player.run_outs = GetRandomValue(0, 4);
                    break;
                    
                case 2: // All-rounder
                    new_player.runs = GetRandomValue(150, 400);
                    new_player.wickets = GetRandomValue(8, 20);
                    new_player.strike_rate = 105.0f + ((float)GetRandomValue(0, 400) / 10.0f);
                    new_player.economy = 7.0f + ((float)GetRandomValue(-20, 40) / 10.0f);
                    new_player.sixes = GetRandomValue(3, 20);
                    new_player.fours = GetRandomValue(10, 50);
                    new_player.catches = GetRandomValue(5, 15);
                    new_player.run_outs = GetRandomValue(1, 5);
                    break;
                    
                case 3: // Wicket-keeper
                    new_player.runs = GetRandomValue(200, 500);
                    new_player.wickets = 0;
                    new_player.strike_rate = 125.0f + ((float)GetRandomValue(0, 450) / 10.0f);
                    new_player.economy = 0.0f;
                    new_player.sixes = GetRandomValue(5, 25);
                    new_player.fours = GetRandomValue(20, 60);
                    new_player.catches = GetRandomValue(15, 30); // Includes wicket-keeping catches
                    new_player.run_outs = GetRandomValue(3, 10); // Includes stumpings
                    break;
            }
            
            // Add player to array
            ++num_players;
            players[num_players / MAX_PLAYERS_PER_TEAM][num_players % MAX_PLAYERS_PER_TEAM] = new_player;
        }
    }
    
    // Make some players foreign players (for IPL rules)
    for (int team_idx = 0; team_idx < num_teams; team_idx++) {
        // Find 4 random players from this team to make foreigners
        int foreign_count = 0;
        while (foreign_count < 4) {
            // Get random player index for this team
            int player_idx = -1;
            for (int i = 0; i < num_players; i++) {
                if (players[i]->team_index == team_idx) {
                    if (GetRandomValue(0, 10) > 7 && !players[i]->is_foreign) {
                        player_idx = i;
                        break;
                    }
                }
            }
            
            // If no player found, find any non-foreign player
            if (player_idx == -1) {
                for (int i = 0; i < num_players; i++) {
                    if (players[i]->team_index == team_idx && !players[i]->is_foreign) {
                        player_idx = i;
                        break;
                    }
                }
            }
            
            // Mark as foreign if found
            if (player_idx != -1) {
                players[player_idx]->is_foreign = true;
                foreign_count++;
            } else {
                break; // Something went wrong, exit the loop
            }
        }
    }
    
    // Ensure one player per team is a captain
    for (int team_idx = 0; team_idx < num_teams; team_idx++) {
        bool captain_assigned = false;
        
        // Try to assign to a non-wicketkeeper first
        for (int i = 0; i < num_players && !captain_assigned; i++) {
            if (players[i]->team_index == team_idx && strcmp(players[i]->role, "Wicket-keeper") != 0) {
                players[i]->is_captain = true;
                captain_assigned = true;
                break;
            }
        }
        
        // If no captain assigned yet, assign to any player
        if (!captain_assigned) {
            for (int i = 0; i < num_players; i++) {
                if (players[i]->team_index == team_idx) {
                    players[i]->is_captain = true;
                    break;
                }
            }
        }
    }
}
