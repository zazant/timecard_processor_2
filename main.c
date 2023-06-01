#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>

#include "process.h"

// Embedding the application manifest
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

#define MAJOR_VERSION 2
#define MINOR_VERSION 0

#define BUTTON_PREFERENCES_ID 1
#define BUTTON_INPUT_FILE_ID 2
#define BUTTON_OUTPUT_FILE_ID 3
#define BUTTON_PROCESS_ID 4

#define CHECKBOX_OVERTIME_POLICY_ID 5

HFONT hFont, hFontBold;

char inputFile[MAX_PATH] = "";
char outputFile[MAX_PATH] = "";

HWND hwndMainWindow;

HWND hwndInputFileButton;
HWND hwndOutputFileButton;
HWND hwndProcessButton;
HWND hwndPreferencesWindow;
HWND hwndOvertimePolicyCheckbox;
HWND hwndVersionText;

BOOL overtimePolicyEnabled;

void EnableProcessButtonIfReady() {
  if (strlen(inputFile) > 0 && strlen(outputFile) > 0) {
    EnableWindow(hwndProcessButton, TRUE);
  }
}

void LoadSettings() {
  overtimePolicyEnabled = GetPrivateProfileInt("Settings", "EndOvertimeAt7PM", 0, ".\\settings.ini");
}

void SaveSettings() {
  char buffer[2];

  _itoa(overtimePolicyEnabled, buffer, 10);
  WritePrivateProfileString("Settings", "EndOvertimeAt7PM", buffer, ".\\settings.ini");
}

BOOL OpenFileDlg(HWND hwnd, char* path, HWND hwndButton, BOOL saveAs) {
  OPENFILENAME ofn;
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrDefExt = "csv";
  ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
  ofn.Flags = saveAs ? 0 : OFN_FILEMUSTEXIST;  // if saveAs is TRUE, it allows selection of non-existing files

  if (saveAs) {
    if (GetSaveFileName(&ofn)) {
      char* filename = PathFindFileName(path);
      SetWindowText(hwndButton, filename);
      EnableProcessButtonIfReady();
      return TRUE;
    }
  } else {
    if (GetOpenFileName(&ofn)) {
      char* filename = PathFindFileName(path);
      SetWindowText(hwndButton, filename);
      EnableProcessButtonIfReady();
      return TRUE;
    }
  }

  return FALSE;
}

void ProcessFiles() {
  // Disable the process button
  EnableWindow(hwndProcessButton, FALSE);

  // Process
  int numEmployees;
  Employee* employees = readEmployees(inputFile, &numEmployees);
  if (employees == NULL) {
    MessageBox(hwndMainWindow, "Error reading input file.", "Error", MB_OK | MB_ICONERROR);
    EnableWindow(hwndProcessButton, TRUE);
    return;
  }

  int readOvertimeResult = readOvertime(employees, numEmployees, "overtime.csv");
  if (readOvertimeResult != 0) {
    MessageBox(hwndMainWindow, "Error reading overtime file.", "Error", MB_OK | MB_ICONERROR);
    free(employees);
    EnableWindow(hwndProcessButton, TRUE);
    return;
  }

  int calcTimeResult = calcTime(employees, numEmployees, overtimePolicyEnabled);
  if (calcTimeResult != 0) {
    MessageBox(hwndMainWindow, "Error calculating time.", "Error", MB_OK | MB_ICONERROR);
    free(employees);
    EnableWindow(hwndProcessButton, TRUE);
    return;
  }

  printEmployees(employees, numEmployees);
  int writeResult = writeEmployees(outputFile, employees, numEmployees);
  if (writeResult != 0) {
    MessageBox(hwndMainWindow, "Error writing output file.", "Error", MB_OK | MB_ICONERROR);
    free(employees);
    EnableWindow(hwndProcessButton, TRUE);
    return;
  }

  // Change the text of the button to "Done! Open file?"
  SetWindowText(hwndProcessButton, "Done! Open file?");

  // Enable the process button
  EnableWindow(hwndProcessButton, TRUE);

  // Enable the output file button to open the file
  EnableWindow(hwndOutputFileButton, TRUE);
}

LRESULT CALLBACK PreferencesWindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_CREATE:
    {
      RECT rect;
      GetWindowRect(hwndMainWindow, &rect);
      int mainWindowX = rect.left;
      int mainWindowY = rect.top;

      int preferencesX = mainWindowX + 10;
      int preferencesY = mainWindowY + 10;

      // Overtime checkbox
      hwndOvertimePolicyCheckbox = CreateWindow("BUTTON", "End overtime at 7PM?", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                                10, 10, 180, 30, hwnd, (HMENU)CHECKBOX_OVERTIME_POLICY_ID, NULL, NULL);
      SendMessage(hwndOvertimePolicyCheckbox, WM_SETFONT, (WPARAM)hFont, TRUE);
      SendMessage(hwndOvertimePolicyCheckbox, BM_SETCHECK, (overtimePolicyEnabled ? BST_CHECKED : BST_UNCHECKED), 0);

      // Divider
      CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ, 10, 46, 180, 2, hwnd, NULL, NULL, NULL);

      // Version text
      char versionText[50];
      sprintf(versionText, "Version %d.%02d", MAJOR_VERSION, MINOR_VERSION);
      hwndVersionText = CreateWindow("STATIC", versionText, WS_VISIBLE | WS_CHILD, 10, 60, 180, 30, hwnd, NULL, NULL, NULL);
      SendMessage(hwndVersionText, WM_SETFONT, (WPARAM)hFont, TRUE);

      // Adjust preferences window size
      SetWindowPos(hwnd, NULL, preferencesX, preferencesY, 212, 130,
                   SWP_NOZORDER);

      // Set window style to disable resizing
      SetWindowLong(hwnd, GWL_STYLE,
                    GetWindowLong(hwnd, GWL_STYLE) & ~WS_THICKFRAME);
    }
    break;
  case WM_CTLCOLORSTATIC: {
    HDC hdcStatic = (HDC)wp;
    if ((HWND)lp == hwndVersionText) {
      SetTextColor(hdcStatic,
                   RGB(100, 100, 100)); // Set the text color to gray
    }
    SetBkColor(hdcStatic,
               GetSysColor(COLOR_BTNFACE)); // Set the background color to
    // default dialog background
    return (LRESULT)GetSysColorBrush(
                                     COLOR_BTNFACE); // Return the default dialog background brush
  } break;
  case WM_COMMAND:
    switch (LOWORD(wp)) {
    case CHECKBOX_OVERTIME_POLICY_ID:
      overtimePolicyEnabled = (SendMessage(hwndOvertimePolicyCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
      SaveSettings();
      break;
    }
    break;
  case WM_CLOSE:
    EnableWindow(hwndMainWindow, TRUE);
    DestroyWindow(hwnd);
    hwndPreferencesWindow = NULL;
    return 0;
  default:
    return DefWindowProcW(hwnd, msg, wp, lp);
  }
  return 0;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch(msg) {
  case WM_CREATE:
    hwndMainWindow = hwnd;
    WNDCLASSW wc = {0};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = ((LPCREATESTRUCT)lp)->hInstance;
    wc.lpszClassName = L"preferencesWindowClass";
    wc.lpfnWndProc = PreferencesWindowProcedure;
    RegisterClassW(&wc);
    HWND hwndPreferencesButton = CreateWindow("BUTTON", "Preferences", WS_VISIBLE | WS_CHILD, 10, 10, 200, 30, hwnd, (HMENU)BUTTON_PREFERENCES_ID, NULL, NULL);
    SendMessage(hwndPreferencesButton, WM_SETFONT, (WPARAM) hFont, TRUE);
    hwndInputFileButton = CreateWindow("BUTTON", "Open input file...", WS_VISIBLE | WS_CHILD, 10, 50, 200, 30, hwnd, (HMENU)BUTTON_INPUT_FILE_ID, NULL, NULL);
    SendMessage(hwndInputFileButton, WM_SETFONT, (WPARAM) hFont, TRUE);
    hwndOutputFileButton = CreateWindow("BUTTON", "Select output file...", WS_VISIBLE | WS_CHILD, 10, 90, 200, 30, hwnd, (HMENU)BUTTON_OUTPUT_FILE_ID, NULL, NULL);
    SendMessage(hwndOutputFileButton, WM_SETFONT, (WPARAM) hFont, TRUE);
    hwndProcessButton = CreateWindow("BUTTON", "Process", WS_VISIBLE | WS_CHILD | WS_DISABLED | BS_DEFPUSHBUTTON, 10, 130, 200, 30, hwnd, (HMENU)BUTTON_PROCESS_ID, NULL, NULL);
    SendMessage(hwndProcessButton, WM_SETFONT, (WPARAM) hFont, TRUE);
    break;
  case WM_COMMAND:
    switch (LOWORD(wp)) {
    case BUTTON_PREFERENCES_ID:
      if (!hwndPreferencesWindow) {
        hwndPreferencesWindow = CreateWindowW(L"preferencesWindowClass", L"Preferences", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
                                              150, 150, 171, 90, NULL, NULL, NULL, NULL);
        EnableWindow(hwnd, FALSE);
      }
      break;
    case BUTTON_INPUT_FILE_ID:
      {
        OpenFileDlg(hwnd, inputFile, hwndInputFileButton, FALSE);
        if (strlen(inputFile) > 0) {
          SendMessage(hwndInputFileButton, WM_SETFONT, (WPARAM) hFontBold, TRUE);
        }
        // Revert the process button text to "Process"
        SetWindowText(hwndProcessButton, "Process");
      }
      break;
    case BUTTON_OUTPUT_FILE_ID:
      {
        char defaultOutputFile[MAX_PATH];
        char originalOutputFile[MAX_PATH];
        strcpy(originalOutputFile, outputFile);
        if (strlen(inputFile) > 0) {
          strcpy(defaultOutputFile, inputFile);
          PathRemoveExtension(defaultOutputFile);
          strcat(defaultOutputFile, "-output.csv");
        } else {
          strcpy(defaultOutputFile, "output.csv");
        }
        strcpy(outputFile, defaultOutputFile);
        if (!OpenFileDlg(hwnd, outputFile, hwndOutputFileButton, TRUE)) {
          strcpy(outputFile, originalOutputFile);
          SetWindowText(hwndOutputFileButton, "Select output file...");
        } else {
          SendMessage(hwndOutputFileButton, WM_SETFONT, (WPARAM) hFontBold, TRUE);
        }
        // Revert the process button text to "Process"
        SetWindowText(hwndProcessButton, "Process");
      }
      break;
    case BUTTON_PROCESS_ID: {
      char buttonText[100];
      GetWindowText(hwndProcessButton, buttonText, sizeof(buttonText));
      if (strcmp(buttonText, "Done! Open file?") == 0) {
        ShellExecute(NULL, "open", outputFile, NULL, NULL, SW_SHOWNORMAL);
      } else {
        ProcessFiles();
      }
    } break;
    }
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProcW(hwnd, msg, wp, lp);
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow)
{
  WNDCLASSW wc = {0};
  wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hInstance = hInst;
  wc.lpszClassName = L"myWindowClass";
  wc.lpfnWndProc = WindowProcedure;
  if(!RegisterClassW(&wc))
    return -1;

  NONCLIENTMETRICS ncm;
  ncm.cbSize = sizeof(ncm);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
  hFont = CreateFontIndirect(&ncm.lfMessageFont);

  LOGFONT lfBold;
  memcpy(&lfBold, &ncm.lfMessageFont, sizeof(LOGFONT));
  lfBold.lfWeight = FW_BOLD;
  hFontBold = CreateFontIndirect(&lfBold);

  // Load settings before creating the window
  LoadSettings();

  hwndMainWindow = CreateWindowW(L"myWindowClass", L"Timecard Processor", 
                                 WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE, 
                                 100, 100, 237, 210, NULL, NULL, NULL, NULL);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  return 0;
}
