#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// =============== CONSTANTS ===============
#define MAX_ESCORTS 100
#define GRAV 9.81 // Gravitational Constant
#define PI 3.14159265358979323846

// ============== MACROS ===================
#define DEG_TO_RAD(d) ((d) * PI / 180.0)
#define RAD_TO_DEG(r) ((r) * 180.0 / PI)

// ============= STRUCTURES ================
typedef struct {
    char typeNotation[4]; 
    char className[40];
    char gunName[50];
    double impactpower;
    double angleRange;
    double minAngle; // stores as degrees
    double maxAngle; // stored as degrees
    int minBulletVelo;
    int maxBulletVelo;
} TypeInfo_E;

typedef struct {
    char name[50];
    char typeNotation;
    char gunName[50];
} TypeInfo_B;

typedef struct {
    int index;
    TypeInfo_E typeInfo;
    int x, y;    // position in canvas
    int alive;   // 1 - Alive ; 0 - Destroyed
} Escortship;

typedef struct {
    char typeNotation;
    double x, y;           // Position on canvas
    double maxVelocity;
    double maxRange;
    int alive;             // 1 = alive, 0 = destroyed
} Battleship;

typedef struct {
    int escortIndex;
    double distance;
    double flightTime;
    double angleUsed;
    double velocityUsed;
} HitRecord;

typedef struct {
    int battleshipSunk;          
    int sunkByEscortIndex;       
    int escortHitCount;          
    HitRecord hits[MAX_ESCORTS]; 
    double battleDuration;
} SimResult;

// ============= GLOBAL VARIABLES ============
int canvasSize = 5000;
int vMax;
int noEscort = 1;
int B_x = 0; 
int B_y = 0;
Escortship E_ships[MAX_ESCORTS];
int selectedBType = 0;    // Index into info_BTypes (set in configureBattleship)

TypeInfo_E info_ETypes[5] = {
    {"EA", "1936A-class Destroyer",   "SK C/34 naval gun",        0.08, 20.0, 0.0, 0.0, 0, 0},
    {"EB", "Gabbiano-class Corvette", "L/47 dual-purpose gun",    0.06, 30.0, 0.0, 0.0, 0, 0},
    {"EC", "Matsu-class Destroyer",   "Type 89 dual-purpose gun", 0.07, 25.0, 0.0, 0.0, 0, 0},
    {"ED", "F-class Escort Ships",    "SK c/32 naval gun",        0.05, 50.0, 0.0, 0.0, 0, 0},
    {"EE", "Japanese Kaibokan",       "(4.7 inch) naval guns",    0.04, 70.0, 0.0, 0.0, 0, 0}
};

int num_ETypes = sizeof(info_ETypes) / sizeof(info_ETypes[0]);

const TypeInfo_B info_BTypes[4] = {
    {"USS Iowa (BB-61)",     'U', "50-caliber Mark 7 gun"},
    {"MS King George V",     'M', "(356 mm) Mark VII gun"},
    {"Richelieu",            'R', "(15 inch) Mle 1935 gun"},
    {"Sovetsky Soyuz-class", 'S', "(16 inch) B-37 gun"}
};

int num_BTypes = sizeof(info_BTypes) / sizeof(info_BTypes[0]);

//============= UTILITY FUNCTIONS ===================

void clearInputBuffer() {
    while (getchar() != '\n');
}

void clearLastLine() {
    printf("\033[1A\033[2K\r");
}

int genRanIntBetween(int min, int max) {
    return min + rand() % (max - min + 1);
}

double genRanDoubleBetween(double min, double max) {
    double scale = (double)rand() / (double)RAND_MAX;
    return min + scale * (max - min);
}

// ============= CORE LOGIC FUNCTIONS ===================

void addETypeValues(int vMax) {
    for (int i = 0; i < num_ETypes; i++) {
        double maximumPossibleMaxAngle = 90.00 - info_ETypes[i].angleRange;
        info_ETypes[i].minAngle = genRanDoubleBetween(0, maximumPossibleMaxAngle);
        info_ETypes[i].maxAngle = info_ETypes[i].minAngle + info_ETypes[i].angleRange;

        info_ETypes[i].maxBulletVelo = genRanIntBetween(1, vMax);
        info_ETypes[i].minBulletVelo = genRanIntBetween(0, info_ETypes[i].maxBulletVelo - 1);

        if (strcmp(info_ETypes[i].typeNotation, "EA") == 0) {
            info_ETypes[i].maxBulletVelo = (int)(1.2 * vMax);
            info_ETypes[i].minBulletVelo = genRanIntBetween(0, info_ETypes[i].maxBulletVelo);
        }
    }
}

void generateEInstances() {
    for (int i = 0; i < noEscort; i++) {
        E_ships[i] = (Escortship){ i, info_ETypes[genRanIntBetween(0, num_ETypes - 1)], genRanIntBetween(0, canvasSize), genRanIntBetween(0, canvasSize), 1 };
    }
}

int saveEInstancesToFile(){
    
    FILE *file_EInstances = fopen("Escort_Instances.txt", "w");
    if (file_EInstances == NULL) {
        printf("Error: Cannot open Escort_Instances file!\n");
        return 1;
    }

    for (int i = 0; i < noEscort; i++) {
        fprintf(file_EInstances, "Index : %d\n\n", E_ships[i].index);
        fprintf(file_EInstances, "Notation : %s\n", E_ships[i].typeInfo.typeNotation);
        fprintf(file_EInstances, "Class : %s\n", E_ships[i].typeInfo.className);
        fprintf(file_EInstances, "Gun : %s\n", E_ships[i].typeInfo.gunName);
        fprintf(file_EInstances, "Impact : %.2f\n", E_ships[i].typeInfo.impactpower);
        fprintf(file_EInstances, "Angle Range : %.2f\n", E_ships[i].typeInfo.angleRange);
        fprintf(file_EInstances, "Min Angle : %.2f\n", E_ships[i].typeInfo.minAngle);
        fprintf(file_EInstances, "MaxAngle : %.2f\n", E_ships[i].typeInfo.maxAngle);
        fprintf(file_EInstances, "Min Bullet Velocity : %d\n", E_ships[i].typeInfo.minBulletVelo);
        fprintf(file_EInstances, "Max Bullet velocity : %d\n\n", E_ships[i].typeInfo.maxBulletVelo);
        fprintf(file_EInstances, "X Coordinate : %d\n", E_ships[i].x);
        fprintf(file_EInstances, "Y Coordinate : %d\n", E_ships[i].y);
        fprintf(file_EInstances, "is Alive : %d\n", E_ships[i].alive);
        fprintf(file_EInstances, "-------------------\n\n\n");
    }

    fclose(file_EInstances);

    return 0;
}

int saveETypeInfoToFile() {
    FILE *file_ETypeInfo = fopen("E_TypeInfo.txt", "w");
    if (file_ETypeInfo == NULL) {
        printf("Error: Cannot open E_TypeInfo file!\n");
        return 1;
    }

    for (int i = 0; i < num_ETypes; i++) {
        fprintf(file_ETypeInfo, "Notation : %s\n", info_ETypes[i].typeNotation);
        fprintf(file_ETypeInfo, "Class : %s\n", info_ETypes[i].className);
        fprintf(file_ETypeInfo, "Gun : %s\n", info_ETypes[i].gunName);
        fprintf(file_ETypeInfo, "Impact : %.2f\n", info_ETypes[i].impactpower);
        fprintf(file_ETypeInfo, "Angle Range : %.2f\n", info_ETypes[i].angleRange);
        fprintf(file_ETypeInfo, "Min Angle : %.2f\n", info_ETypes[i].minAngle);
        fprintf(file_ETypeInfo, "MaxAngle : %.2f\n", info_ETypes[i].maxAngle);
        fprintf(file_ETypeInfo, "Min Bullet Velocity : %d\n", info_ETypes[i].minBulletVelo);
        fprintf(file_ETypeInfo, "Max Bullet velocity : %d\n\n\n", info_ETypes[i].maxBulletVelo);
    }

    fclose(file_ETypeInfo);
    return 0;
}

// ============= UI & INPUT FUNCTIONS ===================

void printHeader() {
    printf("======================================\n");
    printf("|        BATTLESHIP SIMULATOR        |\n");
    printf("|            Part 1 - A              |\n");
    printf("======================================\n\n");
}

void configureArena() {
    printf("========== Configure Arena ===========\n\n");

    while (true) {
        printf("Enter the canvas size of the arena ( 100 - 10000 ) : ");
        if (scanf("%d", &canvasSize) != 1 || (canvasSize < 100 || canvasSize > 10000)) {
            clearLastLine();
            clearInputBuffer();
        } else {
            break;
        }
    }

    while (true) {
        printf("Enter the no.of Escortships in arena ( 1 - %d ) : ", MAX_ESCORTS);
        if (scanf("%d", &noEscort) != 1 || (noEscort > MAX_ESCORTS || noEscort < 1)) {
            clearLastLine();
            clearInputBuffer();
        } else {
            break;
        }
    }
}

void configureBattleship() {
    printf("\n========== Configure Your Battleship ==========\n\n");
    printf("Select your Battleship\n");
    printf("----------------------\n");

    for (int i = 0; i < num_BTypes; i++) {
        TypeInfo_B ship = info_BTypes[i];
        printf("%d. %c - %s\n", (i + 1), ship.typeNotation, ship.name);
    }
    printf("\n");

    bool isValidBType = false;
    while (!isValidBType) {
        int tempBType = -1;
        printf("Enter your choice (1-4): ");
        if (scanf("%d", &tempBType) != 1) {
            tempBType = -1;
        }

        clearInputBuffer();

        switch (tempBType) {
            case 1: case 2: case 3: case 4:
                isValidBType = true;
                selectedBType = tempBType - 1;
                break;
            default:
                clearLastLine();
                break;
        }
    }
    printf("\n");
}

void configureMaxVelocity() {
    while (true) {
        printf("Enter the maximum speed of your Battleship's shell in m/s ( 1 - 200 ): ");
        if (scanf("%d", &vMax) != 1 || vMax <= 0 || vMax > 200) {
            clearLastLine();
            clearInputBuffer();
        } else {
            break;
        }
    }
    printf("\n");
}

void getBPosition() {
    printf("Enter your Battleship's position\n");
    printf("--------------------------------\n");
    printf("1. Enter Manually\n");
    printf("2. Generate Randomly\n\n");

    bool isValidChoice = false;
    int tempChoice = -1;

    while (!isValidChoice) {
        printf("Enter your choice : ");
        if (scanf("%d", &tempChoice) != 1) {
            tempChoice = -1;
        }

        clearInputBuffer();

        switch (tempChoice) {
            case 1:
                isValidChoice = true;
                
                while (true) {
                    printf("X Coordinate ( 0 - %d ) : ", canvasSize);
                    if (scanf("%d", &B_x) != 1 || (B_x > canvasSize || B_x < 0)) {
                        clearLastLine();
                        clearInputBuffer();
                    } else {
                        break;
                    }
                }

                while (true) {
                    printf("Y Coordinate ( 0 - %d ) : ", canvasSize);
                    if (scanf("%d", &B_y) != 1 || (B_y > canvasSize || B_y < 0)) {
                        clearLastLine();
                        clearInputBuffer();
                    } else {
                        break;
                    }
                }
                break;

            case 2:
                isValidChoice = true;
                B_x = genRanIntBetween(0, canvasSize);
                B_y = genRanIntBetween(0, canvasSize);
                printf("Random Position Coordinates ( %d , %d )\n", B_x, B_y);
                break;

            default:
                clearLastLine();
                break;
        }
    }
}

//============= SIMULATION HELPER FUNCTIONS ===================

double calculateDistance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

double calculateRange(double velocity, double angleDeg) {
    double angleRad = DEG_TO_RAD(angleDeg);
    return (velocity * velocity * sin(2.0 * angleRad)) / GRAV;
}

double calculateFlightTime(double velocity, double angleDeg) {
    double angleRad = DEG_TO_RAD(angleDeg);
    return (2.0 * velocity * sin(angleRad)) / GRAV;
}

// Returns the angle in [minAngle, maxAngle] closest to 45 degrees (maximizes range)
double getOptimalAngle(double minAngle, double maxAngle) {
    if (minAngle <= 45.0 && maxAngle >= 45.0) return 45.0;
    if (maxAngle < 45.0) return maxAngle;
    return minAngle;
}

// Returns the angle in [minAngle, maxAngle] farthest from 45 degrees (minimizes range)
double getWorstAngle(double minAngle, double maxAngle) {
    double distMin = fabs(minAngle - 45.0);
    double distMax = fabs(maxAngle - 45.0);
    return (distMin >= distMax) ? minAngle : maxAngle;
}

// Maximum attack range of the Battleship (fires at optimal 45 degrees)
double getMaxRangeB() {
    return ((double)vMax * (double)vMax) / GRAV;
}

//Maximum attack range of an Escort ship
double getMaxRangeE(int idx) {
    TypeInfo_E *info = &E_ships[idx].typeInfo;
    double optAngle = getOptimalAngle(info->minAngle, info->maxAngle);
    return calculateRange((double)info->maxBulletVelo, optAngle);
}

// Minimum attack range of an Escort ship
double getMinRangeE(int idx) {
    TypeInfo_E *info = &E_ships[idx].typeInfo;
    if (info->minBulletVelo == 0) return 0.0;
    double worstAngle = getWorstAngle(info->minAngle, info->maxAngle);
    return calculateRange((double)info->minBulletVelo, worstAngle);
}

// Check if Battleship can hit Escort ship at given index
int canBHitEscort(int idx) {
    double dist = calculateDistance((double)B_x, (double)B_y,
                                    (double)E_ships[idx].x, (double)E_ships[idx].y);
    return dist <= getMaxRangeB();
}

// Check if Escort ship can hit Battleship (distance within E's ring-shaped attack range)
int canEscortHitB(int idx) {
    double dist = calculateDistance((double)B_x, (double)B_y,
                                    (double)E_ships[idx].x, (double)E_ships[idx].y);
    double maxRange = getMaxRangeE(idx);
    double minRange = getMinRangeE(idx);
    return dist >= minRange && dist <= maxRange;
}

// Find optimal firing angle and velocity for B to hit a target at given distance
void findBFiringParams(double dist, double *outAngle, double *outVelocity, double *outFlightTime) {
    if (dist <= 0.0001) {
        *outAngle = 45.0;
        *outVelocity = 0.0;
        *outFlightTime = 0.0;
        return;
    }

    //At 45 degrees, v_needed = sqrt(d * g)
    double v45 = sqrt(dist * GRAV);

    if (v45 <= (double)vMax) {
        *outAngle = 45.0;
        *outVelocity = v45;
    } else {
        // Use max velocity and find the low-trajectory angle
        *outVelocity = (double)vMax;
        double sinVal = (dist * GRAV) / ((double)vMax * (double)vMax);
        if (sinVal > 1.0) sinVal = 1.0;
        *outAngle = RAD_TO_DEG(asin(sinVal)) / 2.0;
    }

    *outFlightTime = calculateFlightTime(*outVelocity, *outAngle);
}

// Find firing angle and velocity for E ship to hit B at given distance
// Searches from min angle upward for minimum flight time
void findEFiringParams(int idx, double dist, double *outAngle, double *outVelocity, double *outFlightTime) {
    TypeInfo_E *info = &E_ships[idx].typeInfo;

    if (dist <= 0.0001) {
        *outAngle = info->minAngle;
        *outVelocity = 0.0;
        *outFlightTime = 0.0;
        return;
    }

    // Search from minAngle upward (lower angles = shorter flight time)
    double step = 0.1;
    for (double a = info->minAngle; a <= info->maxAngle; a += step) {
        double s2a = sin(2.0 * DEG_TO_RAD(a));
        if (s2a <= 0.0001) continue;
        double vReq = sqrt((dist * GRAV) / s2a);
        if (vReq >= (double)info->minBulletVelo && vReq <= (double)info->maxBulletVelo) {
            *outAngle = a;
            *outVelocity = vReq;
            *outFlightTime = calculateFlightTime(vReq, a);
            return;
        }
    }

    // Search from maxAngle downward as fallback
    for (double a = info->maxAngle; a >= info->minAngle; a -= step) {
        double s2a = sin(2.0 * DEG_TO_RAD(a));
        if (s2a <= 0.0001) continue;
        double vReq = sqrt((dist * GRAV) / s2a);
        if (vReq >= (double)info->minBulletVelo && vReq <= (double)info->maxBulletVelo) {
            *outAngle = a;
            *outVelocity = vReq;
            *outFlightTime = calculateFlightTime(vReq, a);
            return;
        }
    }

    // Fallback (shouldn't reach here if canEscortHitB returned true)
    double bestAngle = getOptimalAngle(info->minAngle, info->maxAngle);
    *outAngle = bestAngle;
    *outVelocity = (double)info->maxBulletVelo;
    *outFlightTime = calculateFlightTime(*outVelocity, *outAngle);
}

// ============= BATTLE SIMULATION ===================

SimResult simulateBattle() {
    SimResult result;
    result.battleshipSunk = 0;
    result.sunkByEscortIndex = -1;
    result.escortHitCount = 0;
    result.battleDuration = 0.0;

    double maxRangeB = getMaxRangeB();
    double maxFlightTime = 0.0;

    // Step 1: B fires at all E ships in its range
    // B fires first (0 reload time = instant fire advantage)
    // E ships destroyed by B cannot fire back
    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue;

        double dist = calculateDistance((double)B_x, (double)B_y,
                                        (double)E_ships[i].x, (double)E_ships[i].y);

        if (dist <= maxRangeB) {
            double angle, velocity, flightTime;
            findBFiringParams(dist, &angle, &velocity, &flightTime);

            result.hits[result.escortHitCount].escortIndex = i;
            result.hits[result.escortHitCount].distance = dist;
            result.hits[result.escortHitCount].angleUsed = angle;
            result.hits[result.escortHitCount].velocityUsed = velocity;
            result.hits[result.escortHitCount].flightTime = flightTime;
            result.escortHitCount++;

            E_ships[i].alive = 0; // Destroy the escort ship

            if (flightTime > maxFlightTime) {
                maxFlightTime = flightTime;
            }
        }
    }

    // Step 2: Check surviving E ships - can any hit B?
    // Among those that can, the one with shortest flight time sinks B
    double earliestEHitTime = -1.0;
    int sinkingEscort = -1;

    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue; // Already destroyed by B

        if (canEscortHitB(i)) {
            double dist = calculateDistance((double)B_x, (double)B_y,
                                            (double)E_ships[i].x, (double)E_ships[i].y);
            double angle, velocity, flightTime;
            findEFiringParams(i, dist, &angle, &velocity, &flightTime);

            if (earliestEHitTime < 0 || flightTime < earliestEHitTime) {
                earliestEHitTime = flightTime;
                sinkingEscort = i;
            }
        }
    }

    // Step 3: Determine outcome
    if (sinkingEscort >= 0) {
        result.battleshipSunk = 1;
        result.sunkByEscortIndex = sinkingEscort;
        result.battleDuration = (earliestEHitTime > maxFlightTime)
                                    ? earliestEHitTime : maxFlightTime;
    } else {
        result.battleshipSunk = 0;
        result.battleDuration = maxFlightTime;
    }

    return result;
}

// ============= SIMULATION I/O FUNCTIONS ===================

void saveInitialConditions() {
    FILE *file = fopen("Initial_Conditions.txt", "w");
    if (file == NULL) {
        printf("Error: Cannot open Initial_Conditions file!\n");
        return;
    }

    fprintf(file, "========== INITIAL BATTLEFIELD CONDITIONS ==========\n\n");
    fprintf(file, "Canvas Size : %d x %d\n", canvasSize, canvasSize);
    fprintf(file, "Number of Escort Ships : %d\n\n", noEscort);

    fprintf(file, "--- Battleship ---\n");
    fprintf(file, "Name : %s\n", info_BTypes[selectedBType].name);
    fprintf(file, "Notation : %c\n", info_BTypes[selectedBType].typeNotation);
    fprintf(file, "Gun : %s\n", info_BTypes[selectedBType].gunName);
    fprintf(file, "Max Shell Velocity : %d m/s\n", vMax);
    fprintf(file, "Position : (%d, %d)\n", B_x, B_y);
    fprintf(file, "Max Attack Range : %.2f\n\n", getMaxRangeB());

    fprintf(file, "--- Escort Ships ---\n\n");
    for (int i = 0; i < noEscort; i++) {
        fprintf(file, "Escort #%d\n", E_ships[i].index);
        fprintf(file, "  Type : %s (%s)\n", E_ships[i].typeInfo.typeNotation, E_ships[i].typeInfo.className);
        fprintf(file, "  Gun : %s\n", E_ships[i].typeInfo.gunName);
        fprintf(file, "  Position : (%d, %d)\n", E_ships[i].x, E_ships[i].y);
        fprintf(file, "  Impact Power : %.2f\n", E_ships[i].typeInfo.impactpower);
        fprintf(file, "  Angle Range : %.2f - %.2f degrees\n", E_ships[i].typeInfo.minAngle, E_ships[i].typeInfo.maxAngle);
        fprintf(file, "  Velocity Range : %d - %d m/s\n", E_ships[i].typeInfo.minBulletVelo, E_ships[i].typeInfo.maxBulletVelo);
        fprintf(file, "  Max Attack Range : %.2f\n", getMaxRangeE(i));
        fprintf(file, "  Min Attack Range : %.2f\n", getMinRangeE(i));
        fprintf(file, "  Status : Alive\n\n");
    }

    fclose(file);
    printf("Initial conditions saved to 'Initial_Conditions.txt'\n");
}

void saveBattleResults(SimResult result) {
    FILE *file = fopen("Battle_Results.txt", "w");
    if (file == NULL) {
        printf("Error: Cannot open Battle_Results file!\n");
        return;
    }

    fprintf(file, "========== BATTLE SIMULATION RESULTS (Part 1-A) ==========\n\n");
    fprintf(file, "Battle Duration : %.4f seconds\n\n", result.battleDuration);

    if (result.battleshipSunk) {
        fprintf(file, "OUTCOME : Battleship SUNK!\n");
        fprintf(file, "Sunk by Escort #%d (%s - %s)\n\n",
                result.sunkByEscortIndex,
                E_ships[result.sunkByEscortIndex].typeInfo.typeNotation,
                E_ships[result.sunkByEscortIndex].typeInfo.className);
    } else {
        fprintf(file, "OUTCOME : Battleship SURVIVED!\n\n");
    }

    fprintf(file, "Escort Ships Destroyed by Battleship : %d / %d\n\n", result.escortHitCount, noEscort);

    if (result.escortHitCount > 0) {
        fprintf(file, "--- Hit Details ---\n\n");
        for (int i = 0; i < result.escortHitCount; i++) {
            HitRecord hit = result.hits[i];
            fprintf(file, "Hit #%d\n", i + 1);
            fprintf(file, "  Escort Index : %d\n", hit.escortIndex);
            fprintf(file, "  Type : %s (%s)\n",
                    E_ships[hit.escortIndex].typeInfo.typeNotation,
                    E_ships[hit.escortIndex].typeInfo.className);
            fprintf(file, "  Distance : %.2f\n", hit.distance);
            fprintf(file, "  Angle Used : %.2f degrees\n", hit.angleUsed);
            fprintf(file, "  Velocity Used : %.2f m/s\n", hit.velocityUsed);
            fprintf(file, "  Flight Time : %.4f seconds\n\n", hit.flightTime);
        }
    }

    fprintf(file, "\n--- Final Battlefield Status ---\n\n");
    fprintf(file, "Battleship : %s\n", result.battleshipSunk ? "SUNK" : "OPERATIONAL");
    fprintf(file, "Position : (%d, %d)\n\n", B_x, B_y);

    for (int i = 0; i < noEscort; i++) {
        fprintf(file, "Escort #%d (%s) : %s | Position: (%d, %d)\n",
                E_ships[i].index,
                E_ships[i].typeInfo.typeNotation,
                E_ships[i].alive ? "ALIVE" : "DESTROYED",
                E_ships[i].x, E_ships[i].y);
    }
    fprintf(file, "\n");

    fclose(file);
    printf("Battle results saved to 'Battle_Results.txt'\n");
}

void displayBattleResults(SimResult result) {
    printf("\n======================================\n");
    printf("|      BATTLE SIMULATION RESULTS     |\n");
    printf("======================================\n\n");

    printf("Battleship Max Attack Range : %.2f\n\n", getMaxRangeB());

    if (result.battleshipSunk) {
        printf(">>> BATTLESHIP HAS BEEN SUNK! <<<\n\n");
        printf("Sunk by Escort Ship #%d\n", result.sunkByEscortIndex);
        printf("  Type     : %s (%s)\n",
               E_ships[result.sunkByEscortIndex].typeInfo.typeNotation,
               E_ships[result.sunkByEscortIndex].typeInfo.className);
        printf("  Position : (%d, %d)\n",
               E_ships[result.sunkByEscortIndex].x,
               E_ships[result.sunkByEscortIndex].y);
        printf("\n");
    } else {
        printf(">>> BATTLESHIP SURVIVED! <<<\n\n");
    }

    printf("Escort Ships Destroyed : %d / %d\n", result.escortHitCount, noEscort);
    printf("Battle Duration        : %.4f seconds\n\n", result.battleDuration);

    if (result.escortHitCount > 0) {
        printf("--- Escort Ships Hit ---\n\n");
        for (int i = 0; i < result.escortHitCount; i++) {
            HitRecord hit = result.hits[i];
            printf("  #%d Escort #%d (%s)\n", i + 1, hit.escortIndex,
                   E_ships[hit.escortIndex].typeInfo.className);
            printf("     Distance: %.2f | Angle: %.2f deg | Velocity: %.2f m/s | Time: %.4f s\n",
                   hit.distance, hit.angleUsed, hit.velocityUsed, hit.flightTime);
        }
        printf("\n");
    }

    // Show surviving escort ships
    int surviving = 0;
    for (int i = 0; i < noEscort; i++) {
        if (E_ships[i].alive) surviving++;
    }

    if (surviving > 0) {
        printf("--- Surviving Escort Ships: %d ---\n\n", surviving);
        for (int i = 0; i < noEscort; i++) {
            if (E_ships[i].alive) {
                double dist = calculateDistance((double)B_x, (double)B_y,
                                                (double)E_ships[i].x, (double)E_ships[i].y);
                printf("  Escort #%d (%s) at (%d, %d) | Distance: %.2f",
                       E_ships[i].index,
                       E_ships[i].typeInfo.className,
                       E_ships[i].x, E_ships[i].y, dist);
                if (canEscortHitB(i)) {
                    printf(" [CAN reach Battleship]");
                } else {
                    printf(" [Out of range]");
                }
                printf("\n");
            }
        }
        printf("\n");
    }

    // Print final battlefield status
    printf("--- Final Battlefield Status ---\n\n");
    printf("  Battleship : %s | Position: (%d, %d)\n\n", 
           result.battleshipSunk ? "SUNK" : "OPERATIONAL", B_x, B_y);
    for (int i = 0; i < noEscort; i++) {
        printf("  Escort #%d (%s) : %s | Position: (%d, %d)\n",
               E_ships[i].index,
               E_ships[i].typeInfo.typeNotation,
               E_ships[i].alive ? "ALIVE" : "DESTROYED",
               E_ships[i].x, E_ships[i].y);
    }
    printf("\n======================================\n");
}

// ============= MAIN FUNCTION ========================

int main() {
    srand(time(NULL));

    // 1. Initial UI & Configurations
    printHeader();
    configureArena();
    configureBattleship();
    configureMaxVelocity();
    getBPosition();

    // 2. Core Simulation Generation
    addETypeValues(vMax);
    generateEInstances();

    // 3. Save configuration data
    saveETypeInfoToFile();
    saveEInstancesToFile();

    // 4. Save initial battlefield conditions
    saveInitialConditions();

    // 5. Run battle simulation
    printf("\n========== Running Battle Simulation ==========\n");
    SimResult result = simulateBattle();

    // 6. Display and save results
    displayBattleResults(result);
    saveBattleResults(result);

    // 7. Keep program open
    printf("\nPress Enter to exit...");
    clearInputBuffer();
    getchar();

    return 0;
}
