#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NAME_LENGTH 50
#define NUM_DAYS 7
#define NUM_TIMES 6

#define MAX_EMPLOYEES 100

#define WORKDAY_START 510 // 8:30 am
#define WORKDAY_END 1020  // 5:00 pm
#define OVERTIME_END 1140 // 7:00 pm
#define GRACE_PERIOD 15
#define MINIMUM_WORK_HOURS 465 // 7 hours 45 minutes
#define FULL_WORK_HOURS 480    // 8 hours

typedef struct {
  int in_time;
  int out_time;
} WorkPeriod;

typedef struct {
  WorkPeriod work_periods[NUM_TIMES];
  int num_work_periods;
} WorkDay;

typedef struct {
  char name[NAME_LENGTH];
  WorkDay days[NUM_DAYS];
  int sick_leave_minutes;
  int vacation_minutes;
  int overtime_allowed[NUM_DAYS]; // 1 for overtime allowed, 0 for not allowed
  int calculated_time;
  int calculated_overtime;
} Employee;

int parseTime(char *data) {
  int hour, minute;
  char am_pm[3];

  // Skip asterisk, if present
  if (data[0] == '*') {
    data++;
  }

  if (sscanf(data, "%2d:%2d %2s", &hour, &minute, am_pm) == 3) {
    if (strcmp(am_pm, "am") == 0 || strcmp(am_pm, "AM") == 0) {
      if (hour == 12) {
        hour = 0;
      }
    } else if (strcmp(am_pm, "pm") == 0 || strcmp(am_pm, "PM") == 0) {
      if (hour != 12) {
        hour += 12;
      }
    }
  }

  return hour * 60 + minute;
}

int isEmpty(const char *str) {
  while (*str != '\0') {
    if (!isspace((unsigned char)*str) && *str != '\n')
      return 0;
    str++;
  }
  return 1;
}

char *strtok_single(char *str, char const *delims) {
  static char *src = NULL;
  char *p, *ret = 0;

  if (str != NULL)
    src = str;

  if (src == NULL)
    return NULL;

  if ((p = strpbrk(src, delims)) != NULL) {
    *p = 0;
    ret = src;
    src = ++p;
  } else if (*src) {
    ret = src;
    src = NULL;
  }

  return ret;
}

void trim(char *s) {
  char *p = s;
  int l = strlen(p);

  while (isspace(p[l - 1]))
    p[--l] = 0;
  while (*p && isspace(*p))
    ++p, --l;

  memmove(s, p, l + 1);
}

Employee* readEmployees(char* filename, int* numEmployeesPtr) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    printf("Could not open file %s\n", filename);
    return NULL;
  }

  Employee* employees = (Employee *)malloc(sizeof(Employee) * MAX_EMPLOYEES);
  if (employees == NULL) {
    printf("Could not allocate memory for employees\n");
    return NULL;
  }

  int numEmployees = 0;

  char line[256];
  int lineNum = 0;
  int employeeBlockCount = 0;
  int inOutLineCount = 0;
  int inTimeCount = 0;
  int outTimeCount = 0;
  while (fgets(line, sizeof(line), file)) {
    char* data = line + 14;

    if (lineNum % 46 == 0) {
      for (int i = 0; i < NUM_DAYS; ++i) {
        employees[numEmployees].days[i].num_work_periods = 0;
        employees[numEmployees].overtime_allowed[i] = 0;
      }

      employees[numEmployees].calculated_time = 0;
      employees[numEmployees].calculated_overtime = 0;

      employeeBlockCount++;
      inOutLineCount = 0;
      inTimeCount = 0;
      outTimeCount = 0;
    }

    if (lineNum % 46 >= 1 && lineNum % 46 <= 12) {
      // Separate data into individual time entries
      char* timeEntries[NUM_DAYS] = {0}; // array to store the separated time entries
      int day = 0; // index for the current day of the week
      char *token = strtok_single(data, ",");
      int tokenCount = 0; // to count the tokens
      while (token != NULL && day < NUM_DAYS) {
        if (tokenCount >= 2) { // disregard the first two tokens and empty tokens
          if (!isEmpty(token)) {
            timeEntries[day] = token;
          }
          day++;
        }
        token = strtok_single(NULL, ",");
        tokenCount++;
      }

      // IN/OUT time
      for (int i = 0; i < NUM_DAYS; i++) {
        if (timeEntries[i] != NULL) { // check if the time entry is not NULL
          int time = parseTime(timeEntries[i]);
          int pairNum = inOutLineCount / 2;

          if (pairNum >= NUM_TIMES) {
            printf("Error: Exceeded maximum number of IN/OUT pairs (Line %d, Day %d).\n", lineNum, i);
            continue;
          }

          if (inOutLineCount % 2 == 0) {
            // IN time
            employees[numEmployees].days[i].work_periods[pairNum].in_time = time;
            employees[numEmployees].days[i].num_work_periods = pairNum + 1;
          } else {
            // OUT time
            employees[numEmployees].days[i].work_periods[pairNum].out_time = time;
          }
        }
      }

      inOutLineCount++;

      if (lineNum % 46 == 1) {
        char name[NAME_LENGTH];
        int i, j = 0;

        for (i = 0; i < strlen(data); i++) {
          // Replace "   " with ", "
          if (data[i] == ' ' && data[i + 1] == ' ' && data[i + 2] == ' ') {
            name[j] = ',';
            name[j + 1] = ' ';
            i += 2;
            j += 2;
          }
          // Replace "  -  " with ""
          else if (data[i] == ' ' && data[i + 1] == ' ' && data[i + 2] == '-' &&
                   data[i + 3] == ' ' && data[i + 4] == ' ') {
            i += 4;
          } else {
            name[j] = data[i];
            j++;
          }
        }

        name[j] = '\0'; // Terminate the string

        strcpy(employees[numEmployees].name, name);
      }
    } else if (lineNum % 46 == 25) {
      // Sick leave line
      int hours, minutes;
      if (sscanf(data, "%*[^0-9]%d,%d", &hours, &minutes) == 2) {
        employees[numEmployees].sick_leave_minutes = hours * 60 + minutes;
      } else {
        printf("Error processing sick leave line (Line %d).\n", lineNum);
      }
    } else if (lineNum % 46 == 26) {
      // Vacation time line
      int hours, minutes;
      if (sscanf(data, "%*[^0-9]%d,%d", &hours, &minutes) == 2) {
        employees[numEmployees].vacation_minutes = hours * 60 + minutes;
      } else {
        printf("Error processing vacation time line (Line %d).\n", lineNum);
      }
    } else if (lineNum % 46 == 45) {
      // End of employee block
      if (employeeBlockCount > MAX_EMPLOYEES) {
        printf("Error: Exceeded maximum number of employees.\n");
        break;
      }

      employeeBlockCount = 0;
      numEmployees++;
    }

    lineNum++;
  }

  fclose(file);

  *numEmployeesPtr = numEmployees;

  /* printf("Number of employees: %d\n", numEmployees); */
  return employees;
}

void toLowercase(char *str) {
  for (int i = 0; str[i]; i++) {
    str[i] = tolower((unsigned char)str[i]);
  }
}

int readOvertime(Employee* employees, int numEmployees, char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    printf("Could not open file %s\n", filename);
    return -1;
  }

  char line[256];
  int firstLine = 1;
  while (fgets(line, sizeof(line), file)) {
    if (firstLine) {
      firstLine = 0;
      continue;
    }

    char *nameToken = strtok_single(line, "\"");
    nameToken = strtok_single(NULL, "\"");
    if (nameToken == NULL) {
      printf("Invalid line.\n");
      continue;
    }
    // skip the next token (comma)
    strtok_single(NULL, ",");

    // Search for the employee in the array
    Employee *employee = NULL;
    for (int i = 0; i < numEmployees; ++i) {
      char lowerCaseEmployeeName[NAME_LENGTH];
      strcpy(lowerCaseEmployeeName, employees[i].name);
      toLowercase(lowerCaseEmployeeName);

      char lowerCaseNameToken[NAME_LENGTH];
      strcpy(lowerCaseNameToken, nameToken);
      toLowercase(lowerCaseNameToken);

      if (strcmp(lowerCaseEmployeeName, lowerCaseNameToken) == 0) {
        employee = &employees[i];
        break;
      }
    }

    // If the employee doesn't exist, skip this line
    if (employee == NULL) {
      printf("Employee %s not found.\n", nameToken);
      continue;
    }

    // Now process the overtime data
    for (int i = 0; i < NUM_DAYS; ++i) {
      char *overtimeToken = strtok_single(NULL, ",");
      if (overtimeToken != NULL && strlen(overtimeToken) > 0 && overtimeToken[0] != ' ' && overtimeToken[0] != '\n') {
        // If there is a character other than space or newline in the overtime field, we'll count it as overtime allowed
        employee->overtime_allowed[i] = 1;
      } else {
        employee->overtime_allowed[i] = 0;
      }
    }
  }

  fclose(file);
  return 0;
}

int calcTime(Employee* employees, int numEmployees, int endOvertimeAt7PM) {
  // For each employee:
    // For each day worked:
      // For each work_period:
        // Remove/truncate work_periods to fit between WORKDAY_START (8:30am) and the latest work time allowed for the employee.
          // This is 5:00pm if overtime isn't allowed for the employee, 7:00pm if it is and the endOvertimeAt7PM is checked, otherwise midnight.
        // Add to the daily sums for total hours worked and break time
      // If total daily break time is less than 30 minutes and you've worked more than or equal to 6 hours subtract the difference from 30 from the time worked
      // If worked between 7:45 and 8 hours:
        // If punch in time between 8:30am-8:45am set to 8:30am
        // If break time longer than 30 minutes pay for extra break time taken
        // IMPORTANT: only one of the above options should be selected, calc both and select the greatest
      // Add adjusted daily time worked to weekly total.
    // Store calculated time in current Employee struct.
  int i, j, k;
  for (i = 0; i < numEmployees; ++i) {
    Employee *employee = &employees[i];
    printf("-----------------------------\nEmployee Name: %s\n", employee->name);
    employee->calculated_time = 0;
    employee->calculated_overtime = 0;
    for (j = 0; j < NUM_DAYS; ++j) {
      WorkDay *day = &employee->days[j];
      int daily_work_time = 0;
      int daily_break_time = 0;
      int initial_in_time = 0;
      for (k = 0; k < day->num_work_periods; ++k) {
        WorkPeriod *work_period = &day->work_periods[k];
        // Process work periods as per given conditions
        if (work_period->out_time < WORKDAY_START) {
          continue;
        }
        if (work_period->in_time < WORKDAY_START) {
          work_period->in_time = WORKDAY_START;
        }
        if (!initial_in_time && work_period->in_time)
          initial_in_time = work_period->in_time;
        int overtime_end = employee->overtime_allowed[j] ? (endOvertimeAt7PM ? OVERTIME_END : 1440) : WORKDAY_END;
        if (work_period->in_time >= overtime_end || work_period->out_time <= work_period->in_time) {
          continue;
        }
        if (work_period->out_time > overtime_end) {
          work_period->out_time = overtime_end;
        }
        int break_time = 0;
        if (k != 0 &&
            day->work_periods[k-1].out_time &&
            (break_time = work_period->in_time - day->work_periods[k-1].out_time) &&
            break_time > 0)
          daily_break_time += break_time;
        daily_work_time += work_period->out_time - work_period->in_time;
      }
      if (daily_work_time >= 6*60 && daily_break_time < 30) {
        daily_work_time -= 30 - daily_break_time;
        daily_break_time = 30;
      }
      // Process daily work time as per given conditions
      if (daily_work_time >= MINIMUM_WORK_HOURS && daily_work_time < FULL_WORK_HOURS) {
        int early_calc = initial_in_time - WORKDAY_START;
        int break_calc = daily_break_time - 30;
        break_calc = break_calc > 15 ? 15 : break_calc;
        printf("time: %.4f * early_calc: %i\tbreak_calc: %i\n", daily_work_time / 60.0f, early_calc, break_calc);
        if (early_calc > 0 || break_calc > 0)
          daily_work_time += (early_calc > break_calc ? early_calc : break_calc);
      } else {
        printf("time: %.4f\n", daily_work_time / 60.0f);
      }
      employee->calculated_time += daily_work_time;
    }
    // Process weekly work time as per given conditions
    if (employee->calculated_time > 40*60) {
      employee->calculated_overtime = (employee->calculated_time - 40*60) * 1.5;
      employee->calculated_time = 40*60;
    }
  }
  return 0;
}

char *convertMinutesToTime(int total_minutes) {
  int hours = total_minutes / 60;
  int minutes = total_minutes % 60;
  char *time = (char *)malloc(10 * sizeof(char)); // Allocate memory for the string

  if (hours < 12) {
    if (hours == 0)
      hours = 12;
    snprintf(time, 10, "%02d:%02d am", hours, minutes);
  } else {
    if (hours > 12)
      hours -= 12;
    snprintf(time, 10, "%02d:%02d pm", hours, minutes);
  }

  return time;
}

void printEmployees(Employee* employees, int numEmployees) {
  if (!employees) {
    printf("Error reading employee data\n");
    return;
  }

  for (int i = 0; i < numEmployees; i++) {
    printf("Employee Name: %s\n", employees[i].name);

    for (int day = 0; day < NUM_DAYS; day++) {
      printf("Day %d:\n", day + 1);
      for (int period = 0; period < employees[i].days[day].num_work_periods; period++) {
        printf("\tPeriod %d: In - %s, Out - %s\n", period + 1,
               convertMinutesToTime(employees[i]
                                    .days[day]
                                    .work_periods[period]
                                    .in_time),
               convertMinutesToTime(employees[i]
                                    .days[day]
                                    .work_periods[period]
                                    .out_time));
      }
    }

    // Print the overtime allowed days
    printf("Overtime days allowed: ");
    for (int day = 0; day < NUM_DAYS; day++) {
      if (day == 2) // Check for weekend days (Saturday)
        printf("' ");
      printf("%s ", employees[i].overtime_allowed[day] ? "x" : "o");
      if (day == 3) // Check for weekend days (Sunday)
        printf("' ");
    }
    printf("\n");

    printf("Time worked:   %.2f hours\n",
           employees[i].calculated_time / 60.0f);
    printf("Sick time:     %.2f hours\n", employees[i].sick_leave_minutes / 60.0f);
    printf("Vacation time: %.2f hours\n", employees[i].vacation_minutes / 60.0f);

    printf("Overtime:      %.2f hours\n", employees[i].calculated_overtime / 60.0f);

    printf("Total:         %.2f hours\n",
           (employees[i].sick_leave_minutes + employees[i].vacation_minutes +
            employees[i].calculated_time + employees[i].calculated_overtime) /
               60.0f);

    printf("-----------------------------\n");
  }
}

int writeEmployees(char *filename, Employee *employees, int numEmployees) {
  FILE *file = fopen(filename, "wb");

  if (file == NULL) {
    printf("Could not open file %s", filename);
    return -1;
  }

  // Write the header to the CSV file
  fprintf(file, "\"Employee Name\",Time Worked,Sick Time,Vacation Time,Overtime,Total\n");

  // Buffer to store the prepared employee name
  char employee_name[NAME_LENGTH + 2]; // +2 for potential asterisk and null terminator

  // Write the data for each employee
  for (int i = 0; i < numEmployees; ++i) {
    // Check if the employee has any days marked for overtime
    int hasOvertime = 0;
    for (int j = 0; j < NUM_DAYS; ++j) {
      if (employees[i].overtime_allowed[j] == 1) {
        hasOvertime = 1;
        break;
      }
    }

    // Prepare employee name
    if (hasOvertime) {
      snprintf(employee_name, sizeof(employee_name), "*%s", employees[i].name);
    } else {
      strncpy(employee_name, employees[i].name, NAME_LENGTH);
      employee_name[NAME_LENGTH] = '\0'; // ensure null termination
    }

    // Write employee data to the file
    fprintf(file, "\"%s\",%.2f,%.2f,%.2f,%.2f,%.2f\n", employee_name,
            employees[i].calculated_time / 60.0f,
            employees[i].sick_leave_minutes / 60.0f,
            employees[i].vacation_minutes / 60.0f,
            employees[i].calculated_overtime / 60.0f,
            (employees[i].calculated_time + employees[i].sick_leave_minutes + employees[i].vacation_minutes + employees[i].calculated_overtime) / 60.0f);
  }

  fclose(file);
  return 0; // success
}
