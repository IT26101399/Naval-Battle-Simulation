#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// =============== CONSTANTS ===============
#define MAX_ESCORTS 100
#define MAX_POINTS 100
#define GRAV 9.81 
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
    double minAngle; 
    double maxAngle; 
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
    int x, y;    
    int alive;   
} Escortship;

// Newly added to store the random location points (k )
typedef struct {
    int x, y;
} Point;

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
Escortship E_ships[MAX_ESCORTS];
Escortship original_E_ships[MAX_ESCORTS];

int selectedBType = 0;    

// Part 1-B specific variables
int k_iterations = 5; 
int t_jam = 2;        
double theta_min_B = 20.0;
Point b_path[MAX_POINTS];

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

// ============= UTILITY FUNCTIONS ===================

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
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
        E_ships[i] = (Escortship){ i + 1, info_ETypes[genRanIntBetween(0, num_ETypes - 1)], genRanIntBetween(0, canvasSize), genRanIntBetween(0, canvasSize), 1 };
        original_E_ships[i] = E_ships[i]; 
    }
}

void generateBPath() {
    for (int i = 0; i < k_iterations; i++) {
        b_path[i].x = genRanIntBetween(0, canvasSize);
        b_path[i].y = genRanIntBetween(0, canvasSize);
    }
}

void resetBattlefield() {
    for (int i = 0; i < noEscort; i++) {
        E_ships[i] = original_E_ships[i];
    }
}

int saveEInstancesToFile(){
    FILE *file_EInstances = fopen("Escort_Instances.txt", "w");
    if (file_EInstances == NULL) return 1;

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
    if (file_ETypeInfo == NULL) return 1;

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
    printf("|            Part 1 - B              |\n");
    printf("======================================\n\n");
}

void configureArena() {
    printf("========== Configure Arena ===========\n\n");
    while (true) {
        printf("Enter canvas size (100 - 10000): ");
        if (scanf("%d", &canvasSize) != 1 || canvasSize < 100 || canvasSize > 10000) {
            clearLastLine(); clearInputBuffer();
        } else { break; }
    }
    while (true) {
        printf("Enter no. of Escortships (1 - %d): ", MAX_ESCORTS);
        if (scanf("%d", &noEscort) != 1 || noEscort > MAX_ESCORTS || noEscort < 1) {
            clearLastLine(); clearInputBuffer();
        } else { break; }
    }
}

void configureBattleship() {
    printf("\n========== Configure Battleship ==========\n\n");
    for (int i = 0; i < num_BTypes; i++) {
        printf("%d. %c - %s\n", (i + 1), info_BTypes[i].typeNotation, info_BTypes[i].name);
    }
    printf("\n");

    while (true) {
        int temp;
        printf("Select Battleship (1-4): ");
        if (scanf("%d", &temp) == 1 && temp >= 1 && temp <= 4) {
            selectedBType = temp - 1;
            break;
        }
        clearLastLine(); clearInputBuffer();
    }
    printf("\n");

    while (true) {
        printf("Max speed of Battleship's shell in m/s (1 - 200): ");
        if (scanf("%d", &vMax) == 1 && vMax > 0 && vMax <= 200) break;
        clearLastLine(); clearInputBuffer();
    }
    printf("\n");
}

void configurePart1BSimulations() {
    printf("========== Configure Simulations (Part 1-B) ==========\n\n");
    
    while (true) {
        printf("Number of movement iterations 'k' (1 - %d): ", MAX_POINTS);
        if (scanf("%d", &k_iterations) == 1 && k_iterations > 0 && k_iterations <= MAX_POINTS) break;
        clearLastLine(); clearInputBuffer();
    }

    while (true) {
        printf("Iteration 't' where gun jams (0 to %d): ", k_iterations - 1);
        if (scanf("%d", &t_jam) == 1 && t_jam >= 0 && t_jam < k_iterations) break;
        clearLastLine(); clearInputBuffer();
    }

    while (true) {
        printf("Minimum vertical angle after jam (0 - 30 deg): ");
        if (scanf("%lf", &theta_min_B) == 1 && theta_min_B > 0.0 && theta_min_B <= 30.0) break;
        clearLastLine(); clearInputBuffer();
    }
    printf("\n");
}

// ============= SIMULATION HELPER FUNCTIONS ===================

double calculateDistance(double x1, double y1, double x2, double y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

double calculateRange(double velocity, double angleDeg) {
    return (velocity * velocity * sin(2.0 * DEG_TO_RAD(angleDeg))) / GRAV;
}

double calculateFlightTime(double velocity, double angleDeg) {
    return (2.0 * velocity * sin(DEG_TO_RAD(angleDeg))) / GRAV;
}

double getOptimalAngle(double minAngle, double maxAngle) {
    if (minAngle <= 45.0 && maxAngle >= 45.0) return 45.0;
    if (maxAngle < 45.0) return maxAngle;
    return minAngle;
}

double getWorstAngle(double minAngle, double maxAngle) {
    double distMin = fabs(minAngle - 45.0);
    double distMax = fabs(maxAngle - 45.0);
    return (distMin >= distMax) ? minAngle : maxAngle;
}

double getMaxRangeB() {
    return ((double)vMax * (double)vMax) / GRAV;
}

double getMaxRangeE(int idx) {
    TypeInfo_E *info = &E_ships[idx].typeInfo;
    return calculateRange((double)info->maxBulletVelo, getOptimalAngle(info->minAngle, info->maxAngle));
}

double getMinRangeE(int idx) {
    TypeInfo_E *info = &E_ships[idx].typeInfo;
    if (info->minBulletVelo == 0) return 0.0;
    return calculateRange((double)info->minBulletVelo, getWorstAngle(info->minAngle, info->maxAngle));
}

int canEscortHitB(int idx, double bx, double by) {
    double dist = calculateDistance(bx, by, (double)E_ships[idx].x, (double)E_ships[idx].y);
    return dist >= getMinRangeE(idx) && dist <= getMaxRangeE(idx);
}

void findBFiringParams(double dist, double minAngleAllowed, double *outAngle, double *outVelocity, double *outFlightTime) {
    if (dist <= 0.0001) {
        *outAngle = (45.0 >= minAngleAllowed) ? 45.0 : minAngleAllowed;
        *outVelocity = 0.0;
        *outFlightTime = 0.0;
        return;
    }
    double v45 = sqrt(dist * GRAV);
    if (v45 <= (double)vMax) {
        *outAngle = 45.0;
        *outVelocity = v45;
    } else {
        *outVelocity = (double)vMax;
        double sinVal = (dist * GRAV) / ((double)vMax * (double)vMax);
        if (sinVal > 1.0) sinVal = 1.0;
        double lowAngle = RAD_TO_DEG(asin(sinVal)) / 2.0;
        if (lowAngle >= minAngleAllowed) {
            *outAngle = lowAngle;
        } else {
            *outAngle = 90.0 - lowAngle; // High trajectory
        }
    }
    *outFlightTime = calculateFlightTime(*outVelocity, *outAngle);
}

void findEFiringParams(int idx, double dist, double *outAngle, double *outVelocity, double *outFlightTime) {
    TypeInfo_E *info = &E_ships[idx].typeInfo;
    if (dist <= 0.0001) {
        *outAngle = info->minAngle; *outVelocity = 0.0; *outFlightTime = 0.0; return;
    }
    double step = 0.1;
    for (double a = info->minAngle; a <= info->maxAngle; a += step) {
        double s2a = sin(2.0 * DEG_TO_RAD(a));
        if (s2a <= 0.0001) continue;
        double vReq = sqrt((dist * GRAV) / s2a);
        if (vReq >= (double)info->minBulletVelo && vReq <= (double)info->maxBulletVelo) {
            *outAngle = a; *outVelocity = vReq; *outFlightTime = calculateFlightTime(vReq, a); return;
        }
    }
    *outAngle = getOptimalAngle(info->minAngle, info->maxAngle);
    *outVelocity = (double)info->maxBulletVelo;
    *outFlightTime = calculateFlightTime(*outVelocity, *outAngle);
}

// ============= BATTLE SIMULATION ===================

SimResult simulateBattleStep(double bx, double by, double minAngleAllowedB) {
    SimResult result;
    result.battleshipSunk = 0;
    result.sunkByEscortIndex = -1;
    result.escortHitCount = 0;
    result.battleDuration = 0.0;

    double maxRangeB = getMaxRangeB();
    double maxFlightTime = 0.0;

    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue;
        double dist = calculateDistance(bx, by, (double)E_ships[i].x, (double)E_ships[i].y);
        if (dist <= maxRangeB) {
            double angle, velocity, flightTime;
            findBFiringParams(dist, minAngleAllowedB, &angle, &velocity, &flightTime);
            result.hits[result.escortHitCount] = (HitRecord){ E_ships[i].index, dist, flightTime, angle, velocity };
            result.escortHitCount++;
            E_ships[i].alive = 0; 
            if (flightTime > maxFlightTime) maxFlightTime = flightTime;
        }
    }

    double earliestEHitTime = -1.0;
    int sinkingEscort = -1;

    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue; 
        if (canEscortHitB(i, bx, by)) {
            double dist = calculateDistance(bx, by, (double)E_ships[i].x, (double)E_ships[i].y);
            double angle, velocity, flightTime;
            findEFiringParams(i, dist, &angle, &velocity, &flightTime);
            if (earliestEHitTime < 0 || flightTime < earliestEHitTime) {
                earliestEHitTime = flightTime;
                sinkingEscort = E_ships[i].index - 1; // get array index
            }
        }
    }

    if (sinkingEscort >= 0) {
        result.battleshipSunk = 1;
        result.sunkByEscortIndex = sinkingEscort;
        result.battleDuration = (earliestEHitTime > maxFlightTime) ? earliestEHitTime : maxFlightTime;
    } else {
        result.battleshipSunk = 0;
        result.battleDuration = maxFlightTime;
    }

    return result;
}

// ============= SIMULATION I/O FUNCTIONS (VERBOSE) ===================

void saveInitialConditions(int simMode) {
    char filename[50];
    sprintf(filename, "Initial_Conditions_Sim%d.txt", simMode);
    FILE *file = fopen(filename, "w");
    if (file == NULL) return;

    fprintf(file, "========== INITIAL BATTLEFIELD CONDITIONS ==========\n\n");
    fprintf(file, "Simulation Mode : %d\n", simMode);
    fprintf(file, "Canvas Size : %d x %d\n", canvasSize, canvasSize);
    fprintf(file, "Number of Escort Ships : %d\n\n", noEscort);

    fprintf(file, "--- Battleship ---\n");
    fprintf(file, "Name : %s\n", info_BTypes[selectedBType].name);
    fprintf(file, "Notation : %c\n", info_BTypes[selectedBType].typeNotation);
    fprintf(file, "Gun : %s\n", info_BTypes[selectedBType].gunName);
    fprintf(file, "Max Shell Velocity : %d m/s\n", vMax);
    fprintf(file, "Max Attack Range : %.2f\n\n", getMaxRangeB());

    fprintf(file, "--- Escort Ships ---\n\n");
    for (int i = 0; i < noEscort; i++) {
        fprintf(file, "Escort #%d\n", original_E_ships[i].index);
        fprintf(file, "  Type : %s (%s)\n", original_E_ships[i].typeInfo.typeNotation, original_E_ships[i].typeInfo.className);
        fprintf(file, "  Gun : %s\n", original_E_ships[i].typeInfo.gunName);
        fprintf(file, "  Position : (%d, %d)\n", original_E_ships[i].x, original_E_ships[i].y);
        fprintf(file, "  Impact Power : %.2f\n", original_E_ships[i].typeInfo.impactpower);
        fprintf(file, "  Angle Range : %.2f - %.2f degrees\n", original_E_ships[i].typeInfo.minAngle, original_E_ships[i].typeInfo.maxAngle);
        fprintf(file, "  Velocity Range : %d - %d m/s\n", original_E_ships[i].typeInfo.minBulletVelo, original_E_ships[i].typeInfo.maxBulletVelo);
        fprintf(file, "  Max Attack Range : %.2f\n", getMaxRangeE(i));
        fprintf(file, "  Min Attack Range : %.2f\n", getMinRangeE(i));
        fprintf(file, "  Status : Alive\n\n");
    }
    fclose(file);
}

void appendVerboseBattleResults(FILE *file, int step, int bx, int by, double minAngle, SimResult result) {
    fprintf(file, "========== ITERATION %d ==========\n\n", step + 1);
    fprintf(file, "Battleship Position : (%d, %d)\n", bx, by);
    fprintf(file, "Battleship Gun Min Angle Allowed : %.1f degrees\n\n", minAngle);
    fprintf(file, "Battle Duration : %.4f seconds\n\n", result.battleDuration);

    if (result.battleshipSunk) {
        fprintf(file, "OUTCOME : Battleship SUNK!\n");
        fprintf(file, "Sunk by Escort #%d (%s - %s)\n\n",
                E_ships[result.sunkByEscortIndex].index,
                E_ships[result.sunkByEscortIndex].typeInfo.typeNotation,
                E_ships[result.sunkByEscortIndex].typeInfo.className);
    } else {
        fprintf(file, "OUTCOME : Battleship SURVIVED!\n\n");
    }

    fprintf(file, "Escort Ships Destroyed in this iteration: %d\n\n", result.escortHitCount);

    if (result.escortHitCount > 0) {
        fprintf(file, "--- Hit Details ---\n\n");
        for (int i = 0; i < result.escortHitCount; i++) {
            HitRecord hit = result.hits[i];
            int e_idx = hit.escortIndex - 1; // mapping back to array index
            fprintf(file, "Hit #%d\n", i + 1);
            fprintf(file, "  Escort Index : %d\n", E_ships[e_idx].index);
            fprintf(file, "  Type : %s (%s)\n", E_ships[e_idx].typeInfo.typeNotation, E_ships[e_idx].typeInfo.className);
            fprintf(file, "  Distance : %.2f\n", hit.distance);
            fprintf(file, "  Angle Used : %.2f degrees\n", hit.angleUsed);
            fprintf(file, "  Velocity Used : %.2f m/s\n", hit.velocityUsed);
            fprintf(file, "  Flight Time : %.4f seconds\n\n", hit.flightTime);
        }
    }

    fprintf(file, "\n--- Battlefield Status after Iteration %d ---\n\n", step + 1);
    fprintf(file, "Battleship : %s\n", result.battleshipSunk ? "SUNK" : "OPERATIONAL");
    
    for (int i = 0; i < noEscort; i++) {
        fprintf(file, "Escort #%d (%s) : %s | Position: (%d, %d)\n",
                E_ships[i].index, E_ships[i].typeInfo.typeNotation,
                E_ships[i].alive ? "ALIVE" : "DESTROYED", E_ships[i].x, E_ships[i].y);
    }
    fprintf(file, "\n");
}

void displayVerboseBattleResults(SimResult result, int step, int bx, int by, double minAngle, int simMode) {
    printf("\n======================================\n");
    printf("|     SIMULATION %d - ITERATION %d    |\n", simMode, step + 1);
    printf("======================================\n\n");

    printf("Battleship Position : (%d, %d)\n", bx, by);
    printf("Gun Min Angle Allowed : %.1f degrees\n", minAngle);
    printf("Battleship Max Attack Range : %.2f\n\n", getMaxRangeB());

    if (result.battleshipSunk) {
        printf(">>> BATTLESHIP HAS BEEN SUNK! <<<\n\n");
        printf("Sunk by Escort Ship #%d\n", E_ships[result.sunkByEscortIndex].index);
        printf("  Type     : %s (%s)\n",
               E_ships[result.sunkByEscortIndex].typeInfo.typeNotation,
               E_ships[result.sunkByEscortIndex].typeInfo.className);
        printf("  Position : (%d, %d)\n\n",
               E_ships[result.sunkByEscortIndex].x,
               E_ships[result.sunkByEscortIndex].y);
    } else {
        printf(">>> BATTLESHIP SURVIVED! <<<\n\n");
    }

    printf("Escort Ships Destroyed : %d\n", result.escortHitCount);
    printf("Battle Duration        : %.4f seconds\n\n", result.battleDuration);

    if (result.escortHitCount > 0) {
        printf("--- Escort Ships Hit ---\n\n");
        for (int i = 0; i < result.escortHitCount; i++) {
            HitRecord hit = result.hits[i];
            int e_idx = hit.escortIndex - 1; 
            printf("  #%d Escort #%d (%s)\n", i + 1, E_ships[e_idx].index, E_ships[e_idx].typeInfo.className);
            printf("     Distance: %.2f | Angle: %.2f deg | Velocity: %.2f m/s | Time: %.4f s\n",
                   hit.distance, hit.angleUsed, hit.velocityUsed, hit.flightTime);
        }
        printf("\n");
    }

    int surviving = 0;
    for (int i = 0; i < noEscort; i++) {
        if (E_ships[i].alive) surviving++;
    }

    if (surviving > 0) {
        printf("--- Surviving Escort Ships: %d ---\n\n", surviving);
        for (int i = 0; i < noEscort; i++) {
            if (E_ships[i].alive) {
                double dist = calculateDistance((double)bx, (double)by, (double)E_ships[i].x, (double)E_ships[i].y);
                printf("  Escort #%d (%s) at (%d, %d) | Distance: %.2f",
                       E_ships[i].index, E_ships[i].typeInfo.className, E_ships[i].x, E_ships[i].y, dist);
                if (canEscortHitB(i, bx, by)) {
                    printf(" [CAN reach Battleship]\n");
                } else {
                    printf(" [Out of range]\n");
                }
            }
        }
        printf("\n");
    }

    printf("--- Battlefield Status after Iteration %d ---\n\n", step + 1);
    printf("  Battleship : %s | Position: (%d, %d)\n\n", 
           result.battleshipSunk ? "SUNK" : "OPERATIONAL", bx, by);
    for (int i = 0; i < noEscort; i++) {
        printf("  Escort #%d (%s) : %s | Position: (%d, %d)\n",
               E_ships[i].index, E_ships[i].typeInfo.typeNotation,
               E_ships[i].alive ? "ALIVE" : "DESTROYED", E_ships[i].x, E_ships[i].y);
    }
    printf("\n======================================\n");
}

void runVerboseSimulation(int simMode) {
    char filename[50];
    sprintf(filename, "Battle_Results_Sim%d.txt", simMode);
    
    FILE *file = fopen(filename, "w");
    if (!file) { printf("Error opening %s\n", filename); return; }

    fprintf(file, "========== SIMULATION %d FULL RESULTS ==========\n\n", simMode);
    
    resetBattlefield();
    saveInitialConditions(simMode);

    for (int step = 0; step < k_iterations; step++) {
        int bx = b_path[step].x;
        int by = b_path[step].y;
        
        double currentMinAngle = (simMode == 2 && step >= t_jam) ? theta_min_B : 0.0;

        SimResult result = simulateBattleStep(bx, by, currentMinAngle);
        
        displayVerboseBattleResults(result, step, bx, by, currentMinAngle, simMode);
        appendVerboseBattleResults(file, step, bx, by, currentMinAngle, result);
        
        if (result.battleshipSunk) break;
    }

    fclose(file);
    printf("\nResults for Simulation %d saved to '%s'\n", simMode, filename);
}

// ============= MAIN FUNCTION ========================

int main() {
    srand((unsigned int)time(NULL));

    printHeader();
    configureArena();
    configureBattleship();
    configurePart1BSimulations();

    addETypeValues(vMax);
    generateEInstances();
    generateBPath();

    saveETypeInfoToFile();
    saveEInstancesToFile();

    runVerboseSimulation(1); 
    
    printf("\nPress Enter to begin Simulation 2 (Gun Jammed)...\n");
    clearInputBuffer();
    
    runVerboseSimulation(2); 

    printf("\nBoth Simulations Complete! Check the text files for detailed logs.\n");
    printf("Press Enter to exit...");
    getchar();

    return 0;
}
