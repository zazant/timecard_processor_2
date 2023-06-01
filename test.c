#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "process.h"

#define ASSERT(condition, message, args...)                                    \
  ((void)((condition) ||                                                       \
          (fprintf(stderr, message, ##args), exit(EXIT_FAILURE), 0)))

void test_readEmployees() {
  int numEmployees;
  Employee* employees = readEmployees("test_files/input.csv", &numEmployees);

  // Test details for the first employee
  assert(strcmp(employees[0].name, "Alhalaseh, Sandra") == 0);
  assert(employees[0].sick_leave_minutes == 0);
  assert(employees[0].vacation_minutes == 960);
  // Test the first work period for the first day
  assert(employees[0].days[0].work_periods[0].in_time == parseTime("09:17 am"));
  assert(employees[0].days[0].work_periods[0].out_time == parseTime("01:30 pm"));
  // Test the second work period for the first day
  assert(employees[0].days[0].work_periods[1].in_time == parseTime("01:45 pm"));
  assert(employees[0].days[0].work_periods[1].out_time == parseTime("05:00 pm"));

  // Test details for the second employee
  assert(strcmp(employees[1].name, "Austin, Susan") == 0);
  assert(employees[1].sick_leave_minutes == 0);
  assert(employees[1].vacation_minutes == 0);
  // Test the first work period for the first day
  assert(employees[1].days[0].work_periods[0].in_time == parseTime("08:30 am"));
  assert(employees[1].days[0].work_periods[0].out_time == parseTime("01:15 pm"));

  // Test details for the third employee
  assert(strcmp(employees[2].name, "CALABRO, JANET") == 0);
  assert(employees[2].sick_leave_minutes == 0);
  assert(employees[2].vacation_minutes == 480);
  // Test the first work period for the second day
  assert(employees[2].days[1].work_periods[0].in_time == parseTime("08:27 am"));
  assert(employees[2].days[1].work_periods[0].out_time == parseTime("12:27 pm"));

  printf("All tests passed.\n");
}

void test_readOvertime() {
  // Create a small Employee array for testing
  Employee testEmployees[3] = {{.name = "Doe, John",
                                  .sick_leave_minutes = 120,
                                  .vacation_minutes = 480,
                                  .calculated_time = 0,
                                  .calculated_overtime = 0},
                               {.name = "Doe, Jane",
                                .sick_leave_minutes = 60,
                                .vacation_minutes = 240,
                                .calculated_time = 0,
                                .calculated_overtime = 0},
                               {.name = "Smith, Bob",
                                .sick_leave_minutes = 30,
                                .vacation_minutes = 120,
                                .calculated_time = 0,
                                .calculated_overtime = 0}};

  // Expected overtime allowed data after reading from the CSV
  int expectedOvertime[3][NUM_DAYS] = {
    {0, 1, 0, 0, 1, 0, 0},
    {1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0}
  };

  // Assume we have a file "overtime.csv" with the following contents:
  // Employee name,Thu,Fri,Sat,Sun,Mon,Tue,Wed
  // "Doe, John",,x,,,x,,
  // "Doe, Jane",x,x,,,,,
  // "Smith, Bob",,,,,,x,
  readOvertime(testEmployees, 3, "test_files/overtime.csv");

  // Check if overtime_allowed arrays were read correctly
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < NUM_DAYS; ++j) {
      assert(testEmployees[i].overtime_allowed[j] == expectedOvertime[i][j]);
    }
  }

  printf("All tests passed.\n");
}

// todo: need to rewrite
int test_calcTime() {
    // Expected Employee Structure
    typedef struct {
        char name[NAME_LENGTH];
        double time_worked;
        double sick_time;
        double vacation_time;
        double overtime;
        double total;
    } ExpectedEmployee;

    // Expected Results (todo: rewrite)
    ExpectedEmployee expectedEmployees[] = {};

    ExpectedEmployee overtimeEmployees[] = {};

    int numExpectedEmployees = sizeof(expectedEmployees) / sizeof(ExpectedEmployee);

    // Read employees from the CSV file
    int numEmployees;
    Employee* employees = readEmployees("test_files/input.csv", &numEmployees);

    // Calculate time for all employees
    calcTime(employees, numEmployees, 1);

    // Check results by comparing with the expected output
    for (int i = 0; i < numEmployees; ++i) {
        Employee* e = &employees[i];
        ExpectedEmployee* ee = NULL;

        // Find the matching expected employee
        for (int j = 0; j < numExpectedEmployees; ++j) {
            if (strcmp(e->name, expectedEmployees[j].name) == 0) {
                ee = &expectedEmployees[j];
                break;
            }
        }

        // If no matching expected employee was found, skip to the next actual employee
        if (ee == NULL) {
            continue;
        }

        // Use assert to check if the actual and expected values match
        ASSERT(fabs(e->calculated_time / 60.0 - ee->time_worked) < 0.01, "Error: Time worked for %s. Expected %f, got %f.\n", ee->name, ee->time_worked, e->calculated_time / 60.0);
        ASSERT(fabs(e->sick_leave_minutes / 60.0 - ee->sick_time) < 0.01, "Error: Sick time for %s. Expected %f, got %f.\n", ee->name, ee->sick_time, e->sick_leave_minutes / 60.0);
        ASSERT(fabs(e->vacation_minutes / 60.0 - ee->vacation_time) < 0.01, "Error: Vacation time for %s. Expected %f, got %f.\n", ee->name, ee->vacation_time, e->vacation_minutes / 60.0);
        ASSERT(fabs(e->calculated_overtime / 60.0 - ee->overtime) < 0.01, "Error: Overtime for %s. Expected %f, got %f.\n", ee->name, ee->overtime, e->calculated_overtime / 60.0);
        ASSERT(fabs((e->calculated_time + e->sick_leave_minutes + e->vacation_minutes + e->calculated_overtime) / 60.0 - ee->total) < 0.01, "Error: Total for %s. Expected %f, got %f.\n", ee->name, ee->total, (e->sick_leave_minutes + e->calculated_time + e->vacation_minutes + e->calculated_overtime) / 60.0);
    }

    // Clean up allocated memory
    free(employees);

    return 0;
}

void test_writeEmployees() {
  // Create a test array of employees
  Employee employees[2];

  // Employee 1
  strncpy(employees[0].name, "John Doe", NAME_LENGTH);
  employees[0].calculated_time = 480; // 8 hours in minutes
  employees[0].sick_leave_minutes = 60; // 1 hour
  employees[0].vacation_minutes = 120; // 2 hours
  employees[0].calculated_overtime = 30; // 30 minutes of overtime
  for(int i = 0; i < NUM_DAYS; ++i){
    employees[0].overtime_allowed[i] = 1; // Overtime is allowed
  }

  // Employee 2
  strncpy(employees[1].name, "Jane Smith", NAME_LENGTH);
  employees[1].calculated_time = 510; // 8 hours and 30 minutes in minutes
  employees[1].sick_leave_minutes = 0; // No sick leave
  employees[1].vacation_minutes = 0; // No vacation time
  employees[1].calculated_overtime = 0; // No overtime
  for(int i = 0; i < NUM_DAYS; ++i){
    employees[1].overtime_allowed[i] = 0; // Overtime is not allowed
  }

  int numEmployees = 2;
  char* filename = "test_files/test_write.csv";

  // Call writeEmployees and assert that the return value is 0 (success)
  assert(writeEmployees(filename, employees, numEmployees) == 0);

  // Open the file and check the first few lines to ensure they match what is expected
  FILE* file = fopen(filename, "r");
  assert(file != NULL);

  char buffer[1024];
  fgets(buffer, 1024, file); // header line
  assert(strcmp(buffer, "\"Employee Name\",Time Worked,Sick Time,Vacation Time,Overtime,Total\n") == 0);

  fgets(buffer, 1024, file); // line for Employee 1
  assert(strcmp(buffer, "\"*John Doe\",8.00,1.00,2.00,0.50,11.50\n") == 0);

  fgets(buffer, 1024, file); // line for Employee 2
  assert(strcmp(buffer, "\"Jane Smith\",8.50,0.00,0.00,0.00,8.50\n") == 0);

  fclose(file);

  printf("All tests passed.\n");
}

int main() {
  printf("Testing employees reading...\n");
  test_readEmployees();
  printf("Testing overtime reading...\n");
  test_readOvertime();
  printf("Testing calculations...\n");
  test_calcTime();
  printf("Testing employees writing...\n");
  test_writeEmployees();

  printf("=============================\n");

  int numEmployees;
  Employee *employees = readEmployees("test_files/input.csv", &numEmployees);
  readOvertime(employees, numEmployees, "overtime.csv");
  calcTime(employees, numEmployees, 1);
  printEmployees(employees, numEmployees);
  writeEmployees("test_files/test_write_employees.csv", employees, numEmployees);

  return 0;
}
