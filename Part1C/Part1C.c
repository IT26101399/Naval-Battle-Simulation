#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// =============== CONSTANTS ===============
#define MAX_ESCORTS 100
#define MAX_POINTS 100 // Maximum path iterations (k)
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

// Incoming hit tracking for cumulative damage
typedef struct {
    int e_array_idx; // array index of the firing E ship
    double flightTime;
    double impact;
} IncomingHit;

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
Escortship original_E_ships[MAX_ESCORTS]; // To reset between Sim 1 and 2

int selectedBType = 0;    

// Part 1-B/C specific variables
int k_iterations = 5; 
int t_jam = 2;        
double theta_min_B = 20.0; // degrees
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
        original_E_ships[i] = E_ships[i]; // Backup for Simulation 2
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

// ============= UI & INPUT FUNCTIONS ===================

void printHeader() {
    printf("======================================\n");
    printf("|        BATTLESHIP SIMULATOR        |\n");
    printf("|            Part 1 - C              |\n");
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
    printf("========== Configure Simulations (Part 1-B & C) ==========\n\n");
    
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

// ============= BATTLE SIMULATION (Part 1-C Logic) ===================

SimResult simulateBattleStep(double bx, double by, double minAngleAllowedB, double *b_total_damage) {
    SimResult result;
    result.battleshipSunk = 0;
    result.sunkByEscortIndex = -1;
    result.escortHitCount = 0;
    result.battleDuration = 0.0;

    double maxRangeB = getMaxRangeB();
    double maxFlightTimeB = 0.0; // Track the longest time B takes to hit its targets

    // Step 1: Battleship fires at all Escorts in range
    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue;

        double dist = calculateDistance(bx, by, (double)E_ships[i].x, (double)E_ships[i].y);

        if (dist <= maxRangeB) {
            double angle, velocity, flightTime;
            findBFiringParams(dist, minAngleAllowedB, &angle, &velocity, &flightTime);

            result.hits[result.escortHitCount] = (HitRecord){ E_ships[i].index, dist, flightTime, angle, velocity };
            result.escortHitCount++;

            E_ships[i].alive = 0; // Destroy the escort ship instantly

            if (flightTime > maxFlightTimeB) maxFlightTimeB = flightTime;
        }
    }

    // Step 2: Surviving Escorts fire back
    IncomingHit in_hits[MAX_ESCORTS];
    int in_hit_count = 0;

    for (int i = 0; i < noEscort; i++) {
        if (!E_ships[i].alive) continue; 

        if (canEscortHitB(i, bx, by)) {
            double dist = calculateDistance(bx, by, (double)E_ships[i].x, (double)E_ships[i].y);
            double angle, velocity, flightTime;
            findEFiringParams(i, dist, &angle, &velocity, &flightTime);

            in_hits[in_hit_count++] = (IncomingHit){ i, flightTime, E_ships[i].typeInfo.impactpower };
        }
    }

    // Step 3: Sort incoming hits by flight time (ascending) to see which hits B first
    for (int i = 0; i < in_hit_count - 1; i++) {
        for (int j = 0; j < in_hit_count - i - 1; j++) {
            if (in_hits[j].flightTime > in_hits[j+1].flightTime) {
                IncomingHit temp = in_hits[j];
                in_hits[j] = in_hits[j+1];
                in_hits[j+1] = temp;
            }
        }
    }

    // Step 4: Apply damage cumulatively
    double step_max_e_time = 0.0;
    double fatal_time = 0.0;

    for (int i = 0; i < in_hit_count; i++) {
        *b_total_damage += in_hits[i].impact;
        
        // If damage reaches or exceeds 1.0 (100%), B is sunk!
        if (*b_total_damage >= 1.0) {
            result.battleshipSunk = 1;
            result.sunkByEscortIndex = E_ships[in_hits[i].e_array_idx].index;
            fatal_time = in_hits[i].flightTime;
            break; // No need to process remaining hits, B is already sunk
        }
        step_max_e_time = in_hits[i].flightTime;
    }

    // Calculate duration for this step
    if (result.battleshipSunk) {
        result.battleDuration = (fatal_time > maxFlightTimeB) ? fatal_time : maxFlightTimeB;
    } else {
        result.battleDuration = (step_max_e_time > maxFlightTimeB) ? step_max_e_time : maxFlightTimeB;
    }

    return result;
}

// ============= SIMULATION I/O FUNCTIONS ===================

void appendStepResultToFile(FILE *file, int step, int bx, int by, double minAngle, SimResult result, double b_total_damage) {
    fprintf(file, "---------- Iteration %d ----------\n", step + 1);
    fprintf(file, "B Position : (%d, %d)\n", bx, by);
    fprintf(file, "B Gun Min Angle : %.1f degrees\n", minAngle);
    
    if (result.battleshipSunk) {
        fprintf(file, "OUTCOME : Battleship SUNK by Escort #%d!\n", result.sunkByEscortIndex);
        fprintf(file, "Total Damage taken before sinking: %.2f%%\n", b_total_damage * 100.0);
    } else {
        fprintf(file, "OUTCOME : Battleship Survived this step.\n");
        fprintf(file, "Current Total Damage: %.2f%%\n", b_total_damage * 100.0);
    }

    fprintf(file, "Escorts Destroyed : %d\n", result.escortHitCount);
    for (int i = 0; i < result.escortHitCount; i++) {
        HitRecord h = result.hits[i];
        fprintf(file, "  -> Hit Escort #%d | Dist: %.1f | Angle: %.1f | V: %.1f | Flight Time: %.2fs\n", 
                h.escortIndex, h.distance, h.angleUsed, h.velocityUsed, h.flightTime);
    }
    fprintf(file, "Step Battle Duration: %.4f s\n\n", result.battleDuration);
}

void runSimulation(int simMode) {
    char filename[50];
    sprintf(filename, "Simulation_%d_Results.txt", simMode);
    
    FILE *file = fopen(filename, "w");
    if (!file) { printf("Error opening %s\n", filename); return; }

    fprintf(file, "========== SIMULATION %d RESULTS ==========\n\n", simMode);

    resetBattlefield();
    printf("\n--- Running Simulation %d ---\n", simMode);

    double b_total_damage = 0.0; // Cumulative damage tracker for B

    for (int step = 0; step < k_iterations; step++) {
        int bx = b_path[step].x;
        int by = b_path[step].y;
        
        // Apply gun jam logic for Simulation 2
        double currentMinAngle = (simMode == 2 && step >= t_jam) ? theta_min_B : 0.0;

        SimResult result = simulateBattleStep(bx, by, currentMinAngle, &b_total_damage);
        
        appendStepResultToFile(file, step, bx, by, currentMinAngle, result, b_total_damage);
        
        printf("Iteration %d: %s | Damage Taken: %.1f%% | Hits: %d\n", 
               step + 1, result.battleshipSunk ? "SUNK!" : "Survived", 
               b_total_damage * 100.0, result.escortHitCount);

        if (result.battleshipSunk) break;
    }

    // Final outcome logging if B survives all iterations (Part 1-C specific)
    if (b_total_damage < 1.0) {
        fprintf(file, "\n========== FINAL OUTCOME ==========\n");
        fprintf(file, "Battleship SURVIVED all %d iterations!\n", k_iterations);
        fprintf(file, "Total Cumulative Impact Taken: %.2f%%\n", b_total_damage * 100.0);
        
        printf("\n>> Battleship SURVIVED Simulation %d with %.1f%% total damage!\n", simMode, b_total_damage * 100.0);
    }

    fclose(file);
    printf("Results saved to '%s'\n", filename);
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

    runSimulation(1); // Normal path Simulation
    runSimulation(2); // Jammed gun Simulation

    printf("\nSimulations Complete! Check the text files for detailed iteration logs.\n");
    printf("Press Enter to exit...");
    clearInputBuffer();
    getchar();

    return 0;
}
