// feh_pokemon_sdp.cpp
//
// Author: Aadit Bhatia and Pranav Rajesh
// Date: 12/7/25
// Description: A simple Pokémon battle simulator for FEH devices using C++
// Features a main menu, turn-based battles with projectile attacks, and basic OOP design.
//------------------------------------------------------------

#include "FEHLCD.h"
#include "FEHUtility.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

// ----------------------------- CONSTANTS -----------------------------
const int SCREEN_W = 320;      // typical device dimensions
const int SCREEN_H = 240;      // typical device dimensions

// Menu button layout (keeps your original values)
const int BTN_X = 20;
const int BTN_W = 280;
const int BTN_H = 45;
const int BTN_START_Y = 40;
const int BTN_GAP = 10;
const int NUM_MENU_BUTTONS = 4; // Play, Instructions, Statistics, Credits

// Battle button layout (4 big buttons)
// Adjusted so longer labels (name + PP) fit while keeping two columns within SCREEN_W
const int BBTN_W = 152;    // slightly wider
const int BBTN_H = 48;     // slightly taller to accommodate text
const int BBTN_GAP = 6;    // reduce gap so two buttons still fit
const int BBTN_LEFT_X = 6; 
const int BBTN_RIGHT_X = BBTN_LEFT_X + BBTN_W + BBTN_GAP; 
const int BBTN_START_Y = 142; 
// Position status text (HP / Name) near very top of battle window
const int STATUS_TEXT_Y = 8; 
// Height of the status area box (keeps it short so it won't overlap background elements)
const int STATUS_TEXT_H = 44;

// Gameplay constants
const int RETREAT_HEAL = 8;
const int MIN_DAMAGE = 1;
const int PROJECTILE_SPEED_MS = 20; // sleep between projectile steps
const int PROJECTILE_STEP_PX = 6;   // pixels per step
const int BUTTON_DEBOUNCE_MS = 120;
const int RESULT_PAUSE_MS = 1100;



/*
    Function: SleepMs
    Inputs: int ms - milliseconds to sleep
    Returns: void
    Purpose: Sleep can either take input in ms or s; this ensures ms usage throughout the project
    Author: Pranav Rajesh
*/
void SleepMs(int ms) 
{ 
    Sleep(ms); 
}

/*
    Function: WaitForTouchRelease
    Inputs: none
    Returns: void
    Purpose: Wait until no touch is present; used to debounce and avoid ghost touches.
    Author: Pranav Rajesh
*/
void WaitForTouchRelease()
{
    int tx, ty;
    while (LCD.Touch(&tx, &ty)) {}
    Sleep(BUTTON_DEBOUNCE_MS);
}

/*
    Function: WaitForCleanPress
    Inputs: int &outX, int &outY (by reference) - returns the touch coordinates
    Returns: void
    Purpose: Wait for release, then wait for new press, capture coords, wait for release again.
    Author: Pranav Rajesh
*/
void WaitForCleanPress(int &outX, int &outY)
{
    int x, y;
    // ensure no current touch
    while (LCD.Touch(&x, &y)) {}
    Sleep(BUTTON_DEBOUNCE_MS);

    // wait for touch
    while (!LCD.Touch(&x, &y)) {}
    outX = x; outY = y;

    // wait for release
    while (LCD.Touch(&x, &y)) {}
    Sleep(BUTTON_DEBOUNCE_MS);
}

/*
    Function: randInt
    Inputs: int a, int b - inclusive range
    Returns: random integer in [a,b]
    Purpose: Convenience random integer generator, making this a function makes the code much more readable. 
    Author: Aadit Bhatia
*/
int randInt(int a, int b) 
{ 
    return a + (std::rand() % (b - a + 1)); 
}

// ----------------------------- UI: Menu (comment blocks) -----------------------------
/*
    Function: DrawMenuButton
    Inputs: const char* label, int index (0-based)
    Returns: void
    Purpose: Draw a menu rectangle and label at the given index (vertical). This is what is used to create the buttons on the main menu
    Author: Aadit Bhatia
*/
void DrawMenuButton(const char* label, int index)
{
    int y = BTN_START_Y + index * (BTN_H + BTN_GAP);
    LCD.DrawRectangle(BTN_X, y, BTN_W, BTN_H);
    LCD.WriteAt(label, BTN_X + 10, y + 12);
}

/*
    Function: DrawMainMenu
    Inputs: none
    Returns: void
    Purpose: Draw the full main menu UI. This function uses DrawMenuButton to create each button on the main menu, plus adding a title and background. 
    Author: Aadit Bhatia
*/
void DrawMainMenu()
{
    LCD.Clear(BLACK);
    LCD.SetFontColor(WHITE);
    LCD.WriteAt("POKÉMON MINI (SDP)", BTN_X, 10);

    DrawMenuButton("1. Play", 0);
    DrawMenuButton("2. Instructions", 1);
    DrawMenuButton("3. Statistics", 2);
    DrawMenuButton("4. Credits", 3);

    LCD.Update();
}

/*
    Function: HighlightMenuButton
    Inputs: int index (0-based)
    Returns: void
    Purpose: Visually highlight a menu button briefly for feedback. This makes the game more accessible for those with visual impairments.
    Author: Aadit Bhatia
*/
void HighlightMenuButton(int index)
{
    if (index < 0 || index >= NUM_MENU_BUTTONS) return;
    int y = BTN_START_Y + index * (BTN_H + BTN_GAP);
    LCD.SetFontColor(BLACK);
    LCD.FillRectangle(BTN_X, y, BTN_W, BTN_H);
    LCD.SetFontColor(WHITE);
    //breaks needed so each case doesn't play in order
    switch(index) {
        case 0: LCD.WriteAt("1. Play", BTN_X + 10, y + 12); break;
        case 1: LCD.WriteAt("2. Instructions", BTN_X + 10, y + 12); break;
        case 2: LCD.WriteAt("3. Statistics", BTN_X + 10, y + 12); break;
        case 3: LCD.WriteAt("4. Credits", BTN_X + 10, y + 12); break;
    }
    LCD.Update();
    SleepMs(160);
}

/*
    Function: GetMenuButtonPressed
    Inputs: none
    Returns: int (1..NUM_MENU_BUTTONS) or 0 if invalid
    Purpose: Read a touch and map it robustly to the menu buttons. Recieves whatever button the user pressed based on x,y coords, and maps it to a predetermined
    menu button. 
    Author: Aadit Bhatia
*/
int GetMenuButtonPressed()
{
    int x,y;
    // wait for touch
    while (!LCD.Touch(&x,&y)) {}
    int touchX = x, touchY = y;
    WaitForTouchRelease();

    // filter horizontally
    if (touchX < BTN_X || touchX > (BTN_X + BTN_W)) return 0;

    int rel = touchY - BTN_START_Y;
    if (rel < 0) return 0;
    int rowH = BTN_H + BTN_GAP;
    int index = rel / rowH;
    if (index >= 0 && index < NUM_MENU_BUTTONS) {
        int top = BTN_START_Y + index * rowH;
        int bottom = top + BTN_H;
        if (touchY >= top && touchY <= bottom) return index + 1;
    }
    return 0;
}

// ----------------------------- OOP CLASSES (with comment blocks) -----------------------------
/*
    Class: Move
    Members:
      - string name: human-readable move name
      - int power: a base power used in damage calculation
      - int accuracy: percentage 0..100
      - int pp: number of times move can be used
    Author: Aadit Bhatia
*/
struct Move {
    string name;
    int power;
    int accuracy;
    int pp;
};

/*
    Class: Pokemon
    Members:
      - string name
      - int maxHP, hp
      - int attack, defense
      - vector<Move> moves
      - int x,y,w,h : drawn bounding box (used for simple sprite and collisions)
      - bool defending : whether defend is active
    Methods:
      - reset() : restores hp and clears defend
    Author: Aadit Bhatia
*/
struct Pokemon {
    string name;
    int maxHP;
    int hp;
    int attack;
    int defense;
    vector<Move> moves;
    int x, y, w, h; // for drawing / collision
    bool defending;
    void reset() { hp = maxHP; defending = false; for(auto &m : moves) if (m.pp < 0) m.pp = 0; }
    bool fainted() const { return hp <= 0; }
};

/*
    Class: Player
    Members:
      - string label ("Player 1"/"Player 2")
      - bool isHuman
      - Pokemon pkmn
      - int wins (session stat)
    Author: Pranav Rajesh
*/
struct Player {
    string label;
    bool isHuman;
    Pokemon pkmn;
    int wins;
    Player(): wins(0) {}
};

/*
    Class: Game
    Members:
      - vector<Pokemon> bank : available Pokémon
      - Player p1, p2
      - int gamesPlayed, humanWins, cpuWins
      - int difficulty (0=Easy,1=Hard)
    Methods:
      - loadBank() : populates bank with Pokemon to be randomly chosen
      - setupMatch(difficulty) : assigns players/mon
      - runMatch() : runs a single match loop and returns whether to replay
    Author: Aadit Bhatia and Pranav Rajesh
    Outside Sources: Learned C++ Lambda Functions from W3Schools + my dad uses them for AWS work so he explained the logic to me
    Also learned vectors(dynamic arrays) from W3Schools.
*/
class Game {
public:
    vector<Pokemon> bank;
    Player p1, p2;
    int gamesPlayed;
    int humanWins;
    int cpuWins;
    int difficulty; // 0 easy, 1 hard

    Game(): gamesPlayed(0), humanWins(0), cpuWins(0), difficulty(0) { loadBank(); }

    // loadBank: fill bank with sample Pokémon (stats simplified)
    //Author: Aadit Bhatia
    void loadBank()
    {
        bank.clear();
        // create helper lambda for moves
        //used a lambda function to reduce code duplication and make it easier to read
        auto mk = [](const string &n, int hp, int atk, int def,
                     const Move &m1, const Move &m2, const Move &m3) -> Pokemon {
            Pokemon p; p.name = n; p.maxHP = hp; p.hp = hp; p.attack = atk; p.defense = def;
            p.moves = { m1, m2, m3 };
            p.w = 48; p.h = 48; p.x = 0; p.y = 0; p.defending = false;
            return p;
        };
        bank.push_back(mk("Pikachu", 40, 11, 6, Move{"Thunder",40,95,15}, Move{"Quick",40,100,20}, Move{"Growl",0,100,25}));
        bank.push_back(mk("Charmander",45,10,7, Move{"Ember",40,95,15}, Move{"Scratch",35,100,25}, Move{"Tail",0,100,25}));
        bank.push_back(mk("Squirtle",50,9,9, Move{"Water",40,95,15}, Move{"Tackle",40,100,25}, Move{"Withdraw",0,100,25}));
        bank.push_back(mk("Bulbasaur",48,9,8, Move{"Vine",45,100,15}, Move{"Tackle",40,100,25}, Move{"Seed",0,90,20}));
        bank.push_back(mk("Gengar",55,12,6, Move{"Shadow",50,90,12}, Move{"Lick",30,95,20}, Move{"Hypno",0,70,8}));
        bank.push_back(mk("Onix",60,11,12, Move{"RockT",50,90,15}, Move{"Tackle",40,100,25}, Move{"Harden",0,100,20}));
    }

    /*
        Function: assignPlayers
        Inputs: none
        Returns: void
        Purpose: Randomly decides who is human vs CPU, picks two distinct Pokémon from bank,
                 positions them for drawing on the screen, sets difficulty-dependent seed if needed.
        Author: Pranav Rajesh
    */
    void assignPlayers()
    {
        // randomize who is human: for this project we assign Player1 as human always for clarity,
        // or flip randomly — we'll flip randomly to satisfy random generation requirement
        int assignment = randInt(0,1);
        if (assignment == 0) { p1.isHuman = true; p2.isHuman = false; }
        else                 { p1.isHuman = false; p2.isHuman = true; }

        p1.label = "Player 1";
        p2.label = "Player 2";

        // pick two distinct indices
        // used randInt function to improve readability
        // ensure different Pokémon, should be virtually random.
        int i1 = randInt(0, (int)bank.size()-1);
        int i2 = randInt(0, (int)bank.size()-1);
        while (i2 == i1) i2 = randInt(0, (int)bank.size()-1);

        p1.pkmn = bank[i1];
        p2.pkmn = bank[i2];
        p1.pkmn.reset(); p2.pkmn.reset();

        // set drawing positions (left/right)
        p1.pkmn.x = 40; p1.pkmn.y = 60; // left
        p2.pkmn.x = 220; p2.pkmn.y = 60; // right
    }

    /*
        Function: computeDamage
        Inputs: const Pokemon &att, const Pokemon &def, const Move &m
        Returns: int damage value
        Purpose: Compute damage value based on simple formula and difficulty modifier
        Author: Pranav Rajesh

    */
    int computeDamage(const Pokemon &att, const Pokemon &def, const Move &m)
    {
        double base = (double)att.attack - ((double)def.defense * 0.45);
        if (base < 1.0) base = 1.0;
        double raw = base * (m.power / 20.0);
        double mult = (randInt(85,100) / 100.0);
        // difficulty modifies multiplier: Hard increases CPU damage a bit (we do symmetric effect)
        if (difficulty == 1) raw *= 1.08;
        raw *= mult;
        int dmg = (int)(raw + 0.5);
        if (dmg < MIN_DAMAGE) dmg = MIN_DAMAGE;
        return dmg;
    }

    /*
        Function: getPokemonColor
        Inputs: const string &name
        Purpose: Simple utility to assign a color based on the Pokémon name for sprite
        Author: Aadit Bhatia 
    */
    int getPokemonColor(const string &name) {
        if (name == "Pikachu") return YELLOW;
        if (name == "Charmander") return RED;
        if (name == "Squirtle") return BLUE;
        if (name == "Bulbasaur") return GREEN;
        if (name == "Gengar") return MAGENTA;
        if (name == "Onix") return GRAY;
        return WHITE;
    }


    /*
        Function: drawPokemonGraphic
        Inputs: const Pokemon &p, bool flip (if true, draw mirrored)
        Returns: void
        Purpose: Draws a simple composed graphic representing the Pokémon using shapes.
                 Uses rectangle(s), filled rectangle, and circle (if available).
        Author: Aadit Bhatia
    */
    void drawPokemonGraphic(const Pokemon &p, bool flip=false)
    {
        // background box
        LCD.SetFontColor(WHITE);
        LCD.DrawRectangle(p.x - 6, p.y - 6, p.w + 12, p.h + 12);
        // filled body rectangle as the "sprite"
        LCD.SetFontColor(GREEN); // body color (choice varies)
        LCD.FillRectangle(p.x, p.y, p.w, p.h);

        // small "eye" as a circle or pixel (try DrawCircle, otherwise fall back)
        LCD.SetFontColor(BLACK);
        int cx = p.x + (flip ? p.w/4 : 3*p.w/4);
        int cy = p.y + p.h/4;
        LCD.FillRectangle(cx-2, cy-2, 4, 4);

        // add a little "health bar" on top of box as a filled rectangle 
        int barW = p.w;
        int hpperc = (p.hp * barW) / p.maxHP;
        LCD.SetFontColor(RED);
        LCD.FillRectangle(p.x, p.y - 10, barW, 6);
        LCD.SetFontColor(GREEN);
        LCD.FillRectangle(p.x, p.y - 10, hpperc, 6);
    }

    /*
        Function: drawBackground
        Inputs: none
        Returns: void
        Purpose: Draw a simple background composed of shapes (meets Basic+Advanced Graphics).
                 Uses multiple rectangles and a ground band.
        Author: Pranav Rajesh
    */
    void drawBackground()
    {
        LCD.Clear(BLUE); // sky
        // ground band
        LCD.SetFontColor(BROWN);
        LCD.FillRectangle(0, 160, SCREEN_W, 80);
        // a simple sun (circle or filled rect)
        LCD.SetFontColor(YELLOW);
        // If DrawCircle available, use it; else use FillRectangle as sun
        LCD.FillRectangle(260, 12, 34, 34);
        // horizon line
        LCD.SetFontColor(WHITE);
        LCD.DrawRectangle(10, 10, 60, 30);
    }

    /*
        Function: drawBattleStatus
        Inputs: const Player &p1, const Player &p2
        Returns: void
        Purpose: Draws the HP and name text in the designated status area.
        Author: Pranav Rajesh
    */
    void drawBattleStatus(const Player &p1, const Player &p2) {
        LCD.SetFontColor(BLUE); // Clear the area where status text will go
        LCD.FillRectangle(0, STATUS_TEXT_Y - 5, SCREEN_W, BBTN_START_Y - STATUS_TEXT_Y + 5);
       
        LCD.SetFontColor(WHITE);
        // Player 1 Status (Left)
        LCD.WriteAt((p1.pkmn.name + " (P1)").c_str(), 8, STATUS_TEXT_Y);
        LCD.WriteAt(("HP: " + to_string(p1.pkmn.hp) + "/" + to_string(p1.pkmn.maxHP)).c_str(), 8, STATUS_TEXT_Y + 14);


        // Player 2 Status (Right)
        LCD.WriteAt((p2.pkmn.name + " (P2)").c_str(), 170, STATUS_TEXT_Y);
        LCD.WriteAt(("HP: " + to_string(p2.pkmn.hp) + "/" + to_string(p2.pkmn.maxHP)).c_str(), 170, STATUS_TEXT_Y + 14);


        if (p1.pkmn.defending) LCD.WriteAt("[Defending]", 8, STATUS_TEXT_Y + 28);
        if (p2.pkmn.defending) LCD.WriteAt("[Defending]", 170, STATUS_TEXT_Y + 28);
        LCD.Update();
    }


    /*
        Function: runMatch
        Inputs: none
        Returns: bool (true = player chose to replay immediately)
        Purpose: Runs one full match (turn loop) including animated projectile attacks,
                 uses button UI, handles collisions, and returns whether to automatically replay.
        Author: Aadit Bhatia
        Resources: Learned the use of vectors from W3Schools
    */
    
    bool runMatch()
    {
        // Tracking: current turn (true -> p1)
        bool p1Turn = true;


        // Reset defend states
        p1.pkmn.defending = false; p2.pkmn.defending = false;


        // Battle loop(while both alive)
        while (!p1.pkmn.fainted() && !p2.pkmn.fainted())
        {
            // draw scene: background, status, then pokemon so pokemon render on top of status area
            drawBackground();
            drawBattleStatus(p1, p2);
            drawPokemonGraphic(p1.pkmn, false);
            drawPokemonGraphic(p2.pkmn, true);


            // draw 4 battle buttons (2x2 grid)
            Player *actor = p1Turn ? &p1 : &p2;
            Player *target = p1Turn ? &p2 : &p1;


            // Build button rectangles and labels
            struct Btn
            {
                int x,y,w,h; string label; int id;
            };
            vector<Btn> btns;
           
            // Helper lambda to calculate Y position for row (0 or 1)
            auto getY = [](int row) { return BBTN_START_Y + row * (BBTN_H + BBTN_GAP); };


            // Button 0 (Top Left) - Move 1
            btns.push_back({BBTN_LEFT_X, getY(0), BBTN_W, BBTN_H, actor->pkmn.moves[0].name + " (" + to_string(actor->pkmn.moves[0].pp) + ")", 0});
           
            // Button 1 (Top Right) - Move 2
            btns.push_back({BBTN_RIGHT_X, getY(0), BBTN_W, BBTN_H, actor->pkmn.moves[1].name + " (" + to_string(actor->pkmn.moves[1].pp) + ")", 1});
           
            // Button 2 (Bottom Left) - Move 3
            btns.push_back({BBTN_LEFT_X, getY(1), BBTN_W, BBTN_H, actor->pkmn.moves[2].name + " (" + to_string(actor->pkmn.moves[2].pp) + ")", 2});


            // Button 3 (Bottom Right) - Run
            btns.push_back({BBTN_RIGHT_X, getY(1), BBTN_W, BBTN_H, "RUN", 3});




            // Draw buttons
            // &b is a reference to the button, btns is the whole vector of buttons, so we loop through each button to draw it
            for (auto &b : btns) {
                LCD.SetFontColor(WHITE);
                LCD.DrawRectangle(b.x, b.y, b.w, b.h);
                LCD.WriteAt(b.label.c_str(), b.x + 6, b.y + 12);
            }
            LCD.Update();


            // If actor is human, wait for button press; for CPU, decide action and animate small pause
            int chosen = -1;
            if (actor->isHuman) {
                // wait for clean press and map to button hitboxes
                int tx, ty;
                WaitForCleanPress(tx, ty);


                // find which button was pressed
                //same loop logic as drawing buttons, but now checking if touch coords are within button bounds
                bool found = false;
                for (auto &b : btns) {
                    if (tx >= b.x && tx <= b.x + b.w && ty >= b.y && ty <= b.y + b.h) {
                        chosen = b.id;
                        // highlight visual
                        LCD.SetFontColor(BLACK); LCD.FillRectangle(b.x, b.y, b.w, b.h);
                        LCD.SetFontColor(YELLOW); LCD.WriteAt(b.label.c_str(), b.x + 6, b.y + 12);
                        LCD.Update();
                        SleepMs(160);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // Nothing valid pressed; skip to next iteration
                    continue;
                }
            } else {
                // CPU decision based on difficulty
                SleepMs(400);
                int r = randInt(1,100);
                if (difficulty == 0) { // Easy: more random
                    if (r <= 35) chosen = 0;
                    else if (r <= 70) chosen = 1;
                    else if (r <= 85) chosen = 2; // move 3 (utility)
                    else chosen = 3; // run occasionally
                } else { // Hard: prefer strongest move and attacks
                    int best = 0;
                    for (int i=0;i<(int)actor->pkmn.moves.size();++i) if (actor->pkmn.moves[i].power > actor->pkmn.moves[best].power && actor->pkmn.moves[i].pp>0) best=i;
                    chosen = (randInt(1,100) <= 85) ? best : 3;
                }
                // Highlight CPU chosen button
                if (chosen != -1) {
                    Btn b = btns[chosen];
                    LCD.SetFontColor(BLACK); LCD.FillRectangle(b.x, b.y, b.w, b.h);
                    LCD.SetFontColor(YELLOW); LCD.WriteAt(b.label.c_str(), b.x + 6, b.y + 12);
                    LCD.Update();
                    SleepMs(300);
                }
            }


            // Process chosen action
            if (chosen == 3) { // Run
                // retreat: heal a bit and end match (counts as immediate exit)
                actor->pkmn.hp += RETREAT_HEAL;
                if (actor->pkmn.hp > actor->pkmn.maxHP) actor->pkmn.hp = actor->pkmn.maxHP;
                LCD.Clear(BLACK); LCD.WriteLine((actor->pkmn.name + " retreated and healed.").c_str());
                SleepMs(800);
                // treat retreat as match over and go to menu (no play again)
                return false;
            } else if (chosen != -1) {
                // Attack using move index = chosen (0..2)
                int mIdx = chosen;
                if (mIdx >= (int)actor->pkmn.moves.size()) mIdx = 0;
                Move &mv = actor->pkmn.moves[mIdx];
               
                // If move power is 0, treat as utility/defend
                if (mv.power == 0) {
                    if (mv.pp <= 0) {
                        LCD.Clear(BLACK); LCD.WriteLine("No PP left for that move."); SleepMs(700);
                    } else {
                        // Assume utility move is defend/boost for simplicity
                        actor->pkmn.defending = true;
                        mv.pp--;
                        LCD.Clear(BLACK); LCD.WriteLine((actor->pkmn.name + " used " + mv.name + "! Defending...").c_str());
                        SleepMs(900);
                    }
                } else { // Standard attack move (power > 0)
                    if (mv.pp <= 0) {
                        LCD.Clear(BLACK); LCD.WriteLine("No PP left for that move."); SleepMs(700);
                    } else {
                        // Animate projectile from actor to target and detect collision with bounding box
                        // projectile represented as small filled rectangle that moves across
                        int projX = actor->pkmn.x + (actor == &p1 ? actor->pkmn.w : -8);
                        int projY = actor->pkmn.y + actor->pkmn.h/2;
                        int dir = (actor == &p1) ? 1 : -1;
                        bool hit = false;


                        int roll = randInt(1,100);
                        if (roll > mv.accuracy) {
                            // miss: show message and continue
                            LCD.Clear(BLACK);
                            LCD.WriteLine((actor->pkmn.name + " used " + mv.name + " but missed!").c_str());
                            SleepMs(900);
                        } else {
                            // spawn projectile and animate
                            while (projX > 0 && projX < SCREEN_W) {
                                // Redraw only the dynamic elements (background, status, sprites, projectile)
                                drawBackground();
                                drawBattleStatus(p1, p2); // Ensure status updates (drawn beneath sprites)
                                drawPokemonGraphic(p1.pkmn,false);
                                drawPokemonGraphic(p2.pkmn,true);


                                // draw projectile
                                LCD.SetFontColor(YELLOW);
                                LCD.FillRectangle(projX, projY - 4, 8, 8);


                                // redraw buttons (so user sees them)
                                for (auto &b : btns) { LCD.SetFontColor(WHITE); LCD.DrawRectangle(b.x, b.y, b.w, b.h); LCD.WriteAt(b.label.c_str(), b.x + 6, b.y + 12); }
                                // Re-highlight the chosen button during animation
                                if (chosen != -1) {
                                    Btn b = btns[chosen];
                                    LCD.SetFontColor(BLACK); LCD.FillRectangle(b.x, b.y, b.w, b.h);
                                    LCD.SetFontColor(YELLOW); LCD.WriteAt(b.label.c_str(), b.x + 6, b.y + 12);
                                }


                                LCD.Update();


                                // check collision with target bounding box
                                int tx1 = target->pkmn.x, ty1 = target->pkmn.y, tw = target->pkmn.w, th = target->pkmn.h;
                                int px1 = projX, py1 = projY - 4, pw = 8, ph = 8;
                                bool overlap = !(px1 + pw < tx1 || px1 > tx1 + tw || py1 + ph < ty1 || py1 > ty1 + th);
                                if (overlap) { hit = true; break; }


                                // step projectile
                                projX += dir * PROJECTILE_STEP_PX;
                                SleepMs(PROJECTILE_SPEED_MS);
                            } // end projectile animate


                            // If hit, apply damage
                            if (hit) {
                                // apply damage formula
                                int dmg = computeDamage(actor->pkmn, target->pkmn, mv);
                                if (target->pkmn.defending) {
                                    dmg = (dmg + 1)/2;
                                    target->pkmn.defending = false;
                                }
                                target->pkmn.hp -= dmg; if (target->pkmn.hp < 0) target->pkmn.hp = 0;
                                mv.pp--;
                                // show result
                                LCD.Clear(BLACK);
                                LCD.WriteLine((actor->pkmn.name + " used " + mv.name + "!").c_str());
                                LCD.WriteLine(("Hit for " + to_string(dmg) + " dmg").c_str());
                                SleepMs(900);
                            } else {
                                // projectile flew off screen - treat as miss (shouldn't happen with animation logic)
                                LCD.Clear(BLACK);
                                LCD.WriteLine((actor->pkmn.name + " used " + mv.name + " - no hit.").c_str());
                                mv.pp--;
                                SleepMs(700);
                            }
                        } // end accuracy branch
                    } // end else have PP
                } // end power > 0 (attack)
               
                // If the actor attacked, clear their defend state for next turn. Target state is cleared on hit.
                if (actor->pkmn.defending && mv.power > 0) actor->pkmn.defending = false;


            } // end chosen != -1
           
            // small pause, then swap turns
            SleepMs(200);
            p1Turn = !p1Turn;
        } // end while battle


        // End of battle - display result
        LCD.Clear(BLACK);
        if (p1.pkmn.fainted() && p2.pkmn.fainted()) {
            LCD.WriteLine("It's a tie!");
        } else if (p1.pkmn.fainted()) {
            LCD.WriteLine((p1.label + " lost. " + p2.label + " wins!").c_str());
            if (p2.isHuman) humanWins++; else cpuWins++;
        } else if (p2.pkmn.fainted()) {
            LCD.WriteLine((p2.label + " lost. " + p1.label + " wins!").c_str());
            if (p1.isHuman) humanWins++; else cpuWins++;
        } else {
            LCD.WriteLine("Match ended unexpectedly.");
        }
        gamesPlayed++;
        SleepMs(RESULT_PAUSE_MS);


        // Ask for replay
        LCD.WriteLine("");
        LCD.WriteLine("Play again? Tap TOP half = YES, bottom half = NO");
        int rx, ry;
        WaitForCleanPress(rx, ry);
        bool again = (ry < SCREEN_H/2);
        SleepMs(400);
        return again;
    } // end runMatch


}; // end class Game

// ----------------------------- GLOBAL UI HELPERS (comment blocks) -----------------------------
/*
    Function: DrawTitleAndSmallButtons
    Inputs: Game &game (for showing difficulty)
    Returns: void
    Purpose: Draw small title and a Play submenu that allows picking difficulty (Easy/Hard) and Start.
    Author: Pranav Rajesh
*/
void DrawPlaySubmenu(Game &game)
{
    LCD.Clear(BLACK);
    LCD.SetFontColor(WHITE);
    LCD.WriteAt("Play - Select Difficulty", 24, 10);
    LCD.DrawRectangle(30, 50, 120, 40); LCD.WriteAt("1. Easy", 40, 62);
    LCD.DrawRectangle(170, 50, 120, 40); LCD.WriteAt("2. Hard", 180, 62);
    LCD.DrawRectangle(30, 110, 260, 40); LCD.WriteAt("3. Start Match", 100, 122);
    LCD.Update();
}

/*
    Function: GetSimpleMenuChoice
    Inputs: int numRegions (vertical slices)
    Returns: int region index 1..numRegions
    Purpose: Simple vertical mapping for small submenus (not main menu). this is like difficulty select

*/
int GetSimpleMenuChoice(int numRegions)
{
    int x,y;
    while (!LCD.Touch(&x,&y)) {}
    int ty = y;
    WaitForTouchRelease();
    int regionH = SCREEN_H / numRegions;
    int idx = ty / regionH;
    if (idx < 0) idx = 0;
    if (idx >= numRegions) idx = numRegions - 1;
    return idx + 1;
}

// ----------------------------- MAIN MENU + ENTRY POINT -----------------------------
/*
    Function: showInstructions
    Inputs: none
    Returns: void
    Purpose: Show instructions screen.
    Author: Aadit Bhatia
*/
void showInstructions()
{
    LCD.Clear(BLACK);
    LCD.SetFontColor(WHITE);
    LCD.WriteLine("Instructions:");
    LCD.WriteLine("- Use the 4 in-battle buttons to select moves.");
    LCD.WriteLine("- Attack fires a projectile; defend halves next damage.");
    LCD.WriteLine("- Retreat heals and exits the match.");
    LCD.WriteLine("- Difficulty affects CPU behavior.");
    SleepMs(3000);
}

/*
    Function: showStatistics
    Inputs: Game &game
    Returns: void
    Purpose: Display running session statistics (games played, human wins, CPU wins).

    Author: Aadit Bhatia
*/
void showStatistics(Game &game)
{
    LCD.Clear(BLACK);
    LCD.SetFontColor(WHITE);
    LCD.WriteLine("Statistics (session):");
    LCD.WriteLine(("Games Played: " + to_string(game.gamesPlayed)).c_str());
    LCD.WriteLine(("Human Wins: " + to_string(game.humanWins)).c_str());
    LCD.WriteLine(("CPU Wins: " + to_string(game.cpuWins)).c_str());
    SleepMs(2500);
}

/*
    Function: showCredits
    Inputs: none
    Returns: void
    Purpose: Show project credits.
    Author: Aadit Bhatia
*/
void showCredits()
{
    LCD.Clear(BLACK);
    LCD.SetFontColor(WHITE);
    LCD.WriteLine("Credits:");
    LCD.WriteLine("Project by Aadit Bhatia and Pranav Rajesh");
    SleepMs(2500);
}

/*
    Function: DrawMainMenuButtons
    Inputs: none
    Returns: void
    Purpose: Helper to draw the main menu using the original rectangle button layout.
    Author: Aadit Bhatia
*/
void DrawMainMenuButtons()
{
    LCD.Clear(BLACK);
    LCD.SetFontColor(WHITE);
    LCD.WriteAt("Menu", BTN_X, 10);
    DrawMenuButton("1. Play", 0);
    DrawMenuButton("2. Instructions", 1);
    DrawMenuButton("3. Statistics", 2);
    DrawMenuButton("4. Credits", 3);
    LCD.Update();
}

/*
    Function: mainMenuLoop
    Inputs: reference to Game object
    Returns: void
    Purpose: Main menu loop that responds to touch using large menu buttons and routes to appropriate actions.
    Author: Aadit Bhatia
*/
void mainMenuLoop(Game &game)
{
    bool running = true;
    while (running)
    {
        DrawMainMenuButtons();
        int choice = GetMenuButtonPressed();
        if (choice == 0) continue;
        HighlightMenuButton(choice - 1);
        LCD.Clear(BLACK);
        switch (choice)
        {
            case 1: { // Play: show difficulty submenu, then start matches
                DrawPlaySubmenu(game);
                // Use simple button mapping for sub choices: map raw coordinates to the three rects
                // We'll use WaitForCleanPress and manual box detection
                int sx, sy;
                WaitForCleanPress(sx, sy);
                // easy box: x 30..150, y 50..90
                if (sx >= 30 && sx <= 150 && sy >= 50 && sy <= 90) { game.difficulty = 0; LCD.Clear(BLACK); LCD.WriteLine("Difficulty: EASY"); SleepMs(800); }
                // hard box
                else if (sx >= 170 && sx <= 290 && sy >= 50 && sy <= 90) { game.difficulty = 1; LCD.Clear(BLACK); LCD.WriteLine("Difficulty: HARD"); SleepMs(800); }
                // start match box
                else if (sx >= 30 && sx <= 290 && sy >= 110 && sy <= 150) { LCD.Clear(BLACK); LCD.WriteLine("Starting match..."); SleepMs(600); }
                else { LCD.Clear(BLACK); LCD.WriteLine("No selection, starting default (Easy)."); game.difficulty = 0; SleepMs(700); }

                // assign players & pokemon
                game.assignPlayers();

                // run matches, allow replay as per runMatch return
                bool again = true;
                while (again) {
                    again = game.runMatch();
                    if (again) {
                        // reset both mons HP
                        game.p1.pkmn.reset(); game.p2.pkmn.reset();
                        LCD.Clear(BLACK); LCD.WriteLine("Restarting match..."); SleepMs(700);
                    }
                }
                // return to menu
                LCD.Clear(BLACK);
                LCD.WriteLine("Returning to menu...");
                SleepMs(500);
                break;
            }
            case 2:
                showInstructions();
                break;
            case 3:
                showStatistics(game);
                break;
            case 4:
                showCredits();
                break;
        } // end switch
    } // end while
}

// ----------------------------- MAIN ENTRY POINT -----------------------------
int main(void)
{
    std::srand((unsigned)std::time(nullptr));
    LCD.Clear(BLACK);
    LCD.SetFontColor(WHITE);

    Game game;

    // Main menu loop (option D: menu controls whole app). Exits on Credits->Exit or similar.
    mainMenuLoop(game);

    // Never reached normally; but pause
    while (true) SleepMs(1000);
    return 0;
}





