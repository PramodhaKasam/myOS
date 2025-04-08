#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

// Struct that defines alarm attributes
struct alarm {
    time_t alarm_time;
    int isActive;
};

// Max 10 alarms at a time
struct alarm alarms[10];
int alarm_index = 0;
int clock_running = 0;
time_t custom_time = -1;
int use_custom_time = 0;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

// Format time_t to string YYYY-MM-DD_HH:MM:SS
char* time_to_string(time_t raw_time) {
    if (raw_time == -1) return strdup("Error: Invalid time");
    
    struct tm *local_time = localtime(&raw_time);
    if (!local_time) return strdup("Error: Cannot parse time");

    char* date_time_string = (char*)malloc(30);
    if (!date_time_string) return strdup("Error: Memory allocation failed");

    sprintf(date_time_string, "%04d-%02d-%02d %02d:%02d:%02d",
            local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday,  
            local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

    return date_time_string;
}

// Convert string YYYY-MM-DD_HH:MM:SS to time_t
time_t string_to_time(char* date_time_string) {
    struct tm time_struct = {0};
    if (!strptime(date_time_string, "%Y-%m-%d_%H:%M:%S", &time_struct)) return -1;
    time_struct.tm_isdst = -1; 
    return mktime(&time_struct);
}

// Print and sound alarm
void ring(int alarm_index) {
    printf("\n\U0001F6A8 Alarm #%d says RING RING! \U0001F6A8\n", alarm_index + 1);
}

// Background thread to update custom time
void* update_custom_time(void* arg) {
    while (1) {
        sleep(1);
        if (use_custom_time) {
            pthread_mutex_lock(&time_mutex);
            custom_time += 1;
            pthread_mutex_unlock(&time_mutex);
        }
    }
    return NULL;
}

// Schedule alarm
void schedule() {
    if (alarm_index >= 10) {
        printf("⚠️ Alarm clock is full!\n");
        return;
    }
    
    printf("Set alarm (YYYY-MM-DD_HH:MM:SS): ");
    char input[20];
    scanf("%s", input);

    time_t alarm_time = string_to_time(input);
    pthread_mutex_lock(&time_mutex);
    time_t current_time = use_custom_time ? custom_time : time(NULL);
    pthread_mutex_unlock(&time_mutex);

    if (alarm_time == -1) {
        printf("⚠️ Invalid date format!\n");
        return;
    }

    if (current_time >= alarm_time) {
        printf("⚠️ Cannot set an alarm in the past!\n");
        return;
    }

    alarms[alarm_index] = (struct alarm) { alarm_time, 1 };
    alarm_index++;
    printf("✅ Alarm set successfully!\n");
}

// List alarms
void list() {
    printf("\n📜 Active Alarms 📜\n");
    for (int i = 0; i < alarm_index; i++) {
        if (!alarms[i].isActive) continue;
        char* time_str = time_to_string(alarms[i].alarm_time);
        printf("- Alarm #%d at %s\n", i + 1, time_str);
        free(time_str);
    }
    printf("----------------------\n");
}

// Cancel alarm
void cancel() {
    printf("Enter alarm number to cancel: ");
    int alarm_number;
    scanf("%d", &alarm_number);

    int idx = alarm_number - 1;
    if (idx < 0 || idx >= alarm_index || !alarms[idx].isActive) {
        printf("⚠️ Invalid alarm number!\n");
        return;
    }

    alarms[idx].isActive = 0;
    printf("🛑 Stopped alarm #%d\n", alarm_number);
}

// Handle SIGINT to return to menu from clock
void handle_sigint(int sig) {
    (void)sig;
    clock_running = 0;
}

// Live clock display
void display_clock() {
    signal(SIGINT, handle_sigint);
    clock_running = 1;
    printf("\n⏳ Press 'CTRL + C' to return to menu\n");
    while (clock_running) {
        pthread_mutex_lock(&time_mutex);
        time_t current_time = use_custom_time ? custom_time : time(NULL);
        pthread_mutex_unlock(&time_mutex);
        
        char* time_str = time_to_string(current_time);
        printf("\r⏰ Current Time: %s", time_str);
        fflush(stdout);
        free(time_str);
        
        // Check alarms
        for (int i = 0; i < alarm_index; i++) {
            if (alarms[i].isActive && alarms[i].alarm_time <= current_time) {
                ring(i);
                alarms[i].isActive = 0;
            }
        }

        sleep(1);
    }
    printf("\nReturning to main menu...\n");
}

// Set custom time
void set_custom_time() {
    printf("Enter custom time (YYYY-MM-DD_HH:MM:SS) or 'reset' to use system time: ");
    char input[30];
    scanf("%s", input);

    if (strcmp(input, "reset") == 0) {
        use_custom_time = 0;
        printf("✅ Using system time now.\n");
    } else {
        time_t new_time = string_to_time(input);
        if (new_time == -1) {
            printf("⚠️ Invalid date format!\n");
        } else {
            pthread_mutex_lock(&time_mutex);
            custom_time = new_time;
            use_custom_time = 1;
            pthread_mutex_unlock(&time_mutex);
            printf("✅ Custom time set!\n");
        }
    }
}

// Main function
int main() {
    pthread_t time_thread;
    pthread_create(&time_thread, NULL, update_custom_time, NULL);
    
    char input;
    while (1) {
        printf("\n🔹 Options: [s] Schedule | [l] List | [c] Cancel | [d] Display Clock | [t] Set Time | [x] Exit\n");
        printf("Enter choice: ");
        scanf(" %c", &input);

        switch (input) {
            case 's': schedule(); break;
            case 'l': list(); break;
            case 'c': cancel(); break;
            case 'd': display_clock(); break;
            case 't': set_custom_time(); break;
            case 'x': printf("👋 Goodbye!\n"); return 0;
            default: printf("❌ Invalid option. Try again!\n");
        }
    }
}
