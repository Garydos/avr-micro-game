#include "avr.h"
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_ENEMIES 3
#define SEED_REFRESH_RATE 10
#define ROW_MAX 2
#define SPAWN_SPEED 4
#define MONEY_SPAWN_AREA 14
#define GAME_OVER_TRANSITION_SPEED 50

unsigned short seed = 1;
unsigned short seedTimer = 0;

unsigned char playerChar = 0x40;
unsigned char enemyChar = 0xFF;
unsigned char moneyChar = 0x24;

short speed[] = {6, 5, 4};

typedef struct Point {
    char x; char y;
} Point;

typedef struct Enemy {
    Point loc;
    unsigned char speed;
    unsigned char direction;
    short speedTimer;
} Enemy;

char playerAlive = 1;
char playerHasMoney = 0;
char gameOverTransitionTimer = 0;
char state = 0;


Point player;
Point money;
Enemy enemies[MAX_ENEMIES];
char lenEnemies = 0;
char enemyUpperRowCount = 0;
char enemyLowerRowCount = 0;

char previousKey = 0;
char stepsSinceLastEnemy = 0;

char equalPoints(Point p1, Point p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

char detectEnemyCollision() {
    for (char i = 0; i < lenEnemies; ++i)
        if (equalPoints(enemies[i].loc, player))
            return 1;
    return 0;
}

char detectMoneyCollision() {
    return equalPoints(money, player);
}

unsigned char getRowCount(char row) {
    switch (row) {
        case 0:
            return enemyUpperRowCount;
        case 1:
            return enemyLowerRowCount;
        default:
            return -1;
    }
}

void incrementRowCount(char row) {
    switch (row) {
        case 0:
            ++enemyUpperRowCount;
            break;
        case 1:
            ++enemyLowerRowCount;
            break;
        default:
            break;
    }
}
void decrementRowCount(char row) {
    switch (row) {
        case 0:
            --enemyUpperRowCount;
            break;
        case 1:
            --enemyLowerRowCount;
            break;
        default:
            break;
    }
}

unsigned char getRandomXForMoney() {
    char space = 16 - MONEY_SPAWN_AREA;
    return (rand() % (MONEY_SPAWN_AREA - space)) + space;
}

unsigned char getRandomYForMoney() {
    return rand() % 2; 
}

unsigned char getRandomSpeedInRange(unsigned char l, unsigned char h) {
    /* get a random speed in between l and h inclusive */
    unsigned char diff = h - l;
    unsigned char rdiff = rand() % (diff+1);
    return l + rdiff;
}

unsigned char getRandomSpeed() {
    return getRandomSpeedInRange(0,2);
}

unsigned char getRandomHeight() {
    return rand() % 2;
}

unsigned char getRandomDirection() {
    return rand() % 2;
}

char isPressed(char r, char c) {
    DDRC = (1 << (c+4)); /*set the column to be output, all else inputs*/
    PORTC = (1 << r);/*set the row to be a pullup, and output as ground*/
     
    /* wait for everything to stabilize */ 
    NOP(); NOP(); NOP(); NOP(); NOP(); 
    NOP(); NOP(); NOP(); NOP(); NOP();

    if (GET_BIT(PINC,r))
        return 0;
    return 1;
}



char get_key(void) {
    for (char r = 0; r < 4; ++r) {
        for (char c = 0; c < 4; ++c) {
            if (isPressed(r,c)) {
                return r*4+c+1;
            }
        }
    }
    return 0;
}

char getRandomValidCharacter() {
    return (((rand() % 14) + 2) << 4) + (rand() % 16);
}

char getRandomValidCharNoSpaces() {
    char randc = getRandomValidCharacter();
    if (randc == 0x20 || randc == 0xA0)
        ++randc;
    return randc;
}

void incrementPlayerChar() {
    ++playerChar;
    if (playerChar == 0x20 || playerChar == 0xA0 ||
        playerChar == enemyChar || playerChar == moneyChar)
        ++playerChar;
    if (playerChar == 0)
        playerChar = 0x40;
}

void displayMenu() {
    char buf1[17], buf2[17];
    char sidebuf[5];
    for (char i = 0; i < 5; ++i)
        sidebuf[i] = getRandomValidCharacter();
    sidebuf[4] = '\0';

    sprintf(buf1, "%s  :^ )  %s", sidebuf, sidebuf);
    sprintf(buf2, "Press 1 to start");

    /* write to lcd */
    clr_lcd();
    pos_lcd(0,0);
    puts_lcd2(buf1);
    pos_lcd(1,0);
    puts_lcd2(buf2);
}

void displayGameOver() {
    char buf1[17], buf2[17];
    char sidebuf[3];
    for (char i = 0; i < 3; ++i)
        sidebuf[i] = getRandomValidCharacter();
    sidebuf[2] = '\0';

    sprintf(buf1, "%s Game Over  %s", sidebuf, sidebuf);
    sprintf(buf2, "Press 1");

    /* write to lcd */
    clr_lcd();
    pos_lcd(0,0);
    puts_lcd2(buf1);
    pos_lcd(1,0);
    puts_lcd2(buf2);
}

void handleMenuInput() {
    char key = get_key();
    if (key == 1 && key != previousKey) {
        state = 1;
    }
    previousKey = key;
}

void resetGame();

void handleGameOverInput() {
    char key = get_key();
    if (key == 1) {
        resetGame();
    }
    previousKey = key;
}

void playMenu() {
    handleMenuInput();
    displayMenu();
}

void playGameOver() {
    handleGameOverInput();
    displayGameOver();
}

void drawMoney() {
    pos_lcd(money.y, money.x);
    put_lcd(moneyChar);
}

void drawPlayer() {
    pos_lcd(player.y, player.x);
    put_lcd(playerChar);
}

void drawEnemies() {
    for (char i = 0; i < lenEnemies; ++i) {
        pos_lcd(enemies[i].loc.y, enemies[i].loc.x);
        put_lcd(enemyChar);
    }
}

void drawObjects() {
    char buf1[17], buf2[17];
    clr_lcd();

    drawMoney();
    drawPlayer();
    drawEnemies();
}


void movePlayer(unsigned char dir) {
    /* dir = 1 -> up
     * dir = 2 -> down
     * dir = 3 -> left
     * dir = 4 -> right */
    switch(dir) {
        case 1:
            --player.y;
            break;
        case 2:
            ++player.y;
            break;
        case 3:
            --player.x;
            break;
        case 4:
            ++player.x;
            break;
        default:
            break;
    }
    if (player.y < 0)
        player.y = 0;
    if (player.x < 0)
        player.x = 0;
    if (player.y > 1)
        player.y = 1;
    if (player.x > 15)
        player.x = 15;
}

void handlePlayerInput() {
    /* get a key press */
    char key = get_key();
    if (key == previousKey)
        return;
    switch (key) {
        case 3:
            movePlayer(1);
            break;
        case 7:
            movePlayer(2);
            break;
        case 6:
            movePlayer(3);
            break;
        case 8:
            movePlayer(4);
            break;
        default:
            break;
    }
    previousKey = key;
}

void seedRNG() {
    unsigned short reading = 1;
    SET_BIT(ADCSRA, 6); 
    while (GET_BIT(ADCSRA,6)) {
        /* wait for the ADC to stop */
    }
    /* get the new ADC reading */
    reading = ADC;
    /* never allow the seed to be 0 */
    seed = reading > 0 ? reading : 1;
    srand(seed);
}

void updateSeed() {
    ++seedTimer;
    if (seedTimer >= SEED_REFRESH_RATE){
        seedRNG();
        seedTimer = 0;
    }
}

Enemy createEnemy(char row, char direction, char sp) {
    Enemy enemy;
    enemy.loc.x = direction == 0 ? 15 : 0;
    enemy.loc.y = row;
    enemy.speed = sp;
    enemy.direction = direction;
    enemy.speedTimer = speed[sp];
    return enemy;
}

void spawnEnemies() {
    if (lenEnemies < MAX_ENEMIES && stepsSinceLastEnemy > SPAWN_SPEED) {
        char row = getRandomHeight();
        if (getRowCount(row) >= ROW_MAX)
            row = row == 0 ? 1 : 0;
        char sp = getRandomSpeed();
        char direction = getRandomDirection();
        enemies[lenEnemies++] = createEnemy(row, direction, sp);
        stepsSinceLastEnemy = 0;
        incrementRowCount(row);
    }
    ++stepsSinceLastEnemy;
}

void removeEnemy(char index) {
    decrementRowCount(enemies[index].loc.y);
    for (char i = index; i < lenEnemies - 1; ++i) {
        enemies[i] = enemies[i+1];
    }
    --lenEnemies;
}

void moveEnemies() {
    for (char i = 0; i < lenEnemies; ++i) {
        Enemy *enemy = &(enemies[i]);
        enemy->speedTimer -= 1;
        if (enemy->speedTimer <= 0) {
            enemy->loc.x += (enemy->direction == 0 ? -1 : 1);
            enemy->speedTimer = speed[enemy->speed];
        }
        if (enemy->loc.x < 0 || enemy->loc.x > 15) {
            removeEnemy(i);
        }
    }
}

void spawnPlayer() {
    player.x = 8;
    player.y = 0;
}

void spawnMoney() {
    if (playerHasMoney){
        money.x = getRandomXForMoney();
        money.y = getRandomYForMoney();
        playerHasMoney = 0; 
    }
}

void handleCollisions() {
    if (detectEnemyCollision()) {
        state = 2;
    }
    if (detectMoneyCollision()) {
        playerHasMoney = 1;
        incrementPlayerChar();
    }
}

void playGame() { 
        handlePlayerInput();
        spawnMoney();
        spawnEnemies();
        moveEnemies();
        drawObjects();
        handleCollisions();
}

void playGameOverTransition() {
    drawObjects();
    ++gameOverTransitionTimer;
    if (gameOverTransitionTimer >= GAME_OVER_TRANSITION_SPEED) {
        gameOverTransitionTimer = 0;
        state = 3;
    }
}

void resetGame() {
    spawnPlayer();
    seedRNG();
    playerHasMoney = 1;
    lenEnemies = 0;
    playerChar = 0x40;
    enemyUpperRowCount = 0;
    enemyLowerRowCount = 0;
    previousKey = 0;
    stepsSinceLastEnemy = 0;
    state = 0;
}


int main() {
    ADMUX = 0x40;
    ADCSRA = 0x80;
    ini_lcd();

    resetGame();
    for(;;) {
        updateSeed();
        switch (state) {
            case 0:
                playMenu();
                break;
            case 1:
                playGame();
                break;
            case 2:
                playGameOverTransition();
                break;
            case 3:
                playGameOver();
        }
        wait_avr(50);
    }
}
