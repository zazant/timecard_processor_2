# old timecard processor code for reference

# truncate all hours between 8:30am and 5:00pm
# lunch has to be 30min if worked > 6hours


import os
import sys
import csv
import subprocess
from datetime import *
from copy import deepcopy
from fbs_runtime.application_context.PyQt5 import ApplicationContext
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *

version = "v1.4.1"

class Employee:
	day_dict = {
		0: "Thursday",
		1: "Friday",
		2: "Saturday",
		3: "Sunday",
		4: "Monday",
		5: "Tuesday",
		6: "Wednesday"
	}

	def __init__(self, init_array, overtime_employees, extra_break, overtime_till_seven):
		# set initial values
		self.times = [[], [], [], [], [], [], []]
		self.time_duos = [[], [], [], [], [], [], []]

		self.worked_time = timedelta(0)

		self.overtime = timedelta(0)
		self.vacation_time = timedelta(0)
		self.sick_time = timedelta(0)
		self.holiday_time = timedelta(0)
		self.extra_time = timedelta(0)

		self.overtime_privilege = False

		self.daily_totals = []

		# format name
		self.name = init_array[1][0].replace("  -  ", "").replace("   ", ", ").title()

		# add overtime privileges if available	
		if self.name in overtime_employees:
			self.overtime_privilege = True
		
		# convert csv clock times to python datetime arrays
		for row in init_array[1:13]:
			for index, element in enumerate(row[2:]):
				if element != '':
					fixed_date = element.replace("*", "")
					self.times[index].append(datetime.strptime(fixed_date, '%I:%M %p'))

		# if amount of clocks in day is odd, fix the day with user input
		times_copy = self.times.copy()
		for day_index, day in enumerate(times_copy):
			if len(day) % 2 == 1:
				self.fix_day(day_index)

		# sort clocks into lists of 2 dimensional duos (clock in, out)
		for day_index, day in enumerate(self.times):
			for clock_index, clock in enumerate(day):
				if clock_index % 2 == 0:
					time_duo = [day[clock_index], day[clock_index + 1]]
					# print(self.name, " ", day_index, " ", time_duo)
					self.time_duos[day_index].append(time_duo)

		# create copy of duos matrix to delete excess duos
		time_duos_copy = deepcopy(self.time_duos)

		# delete excess duos
		for day_index, day in enumerate(time_duos_copy):
			for time_duo_index, time_duo in enumerate(day):
				# get rid of duos that start and end before 8:30
				if time_duo[1] < datetime(hour=8, minute=30, year=1900, month=1, day=1):
					self.time_duos[day_index].remove(time_duo)

				overtime_today = False
				if self.overtime_privilege:
					overtime_today = overtime_employees[self.name][day_index]

				# get rid of duos that start and end after 5 unless employee has overtime privilege
				# else remove duos after 7 unless overtime till seven not on
				if not overtime_today and time_duo[0] > datetime(hour=17, minute=0, year=1900, month=1, day=1):
					self.time_duos[day_index].remove(time_duo)
				elif overtime_today and time_duo[0] > datetime(hour=19, minute=0, year=1900, month=1, day=1) and overtime_till_seven:
					self.time_duos[day_index].remove(time_duo)
		
		# truncate duos
		for day_index, day in enumerate(self.time_duos):
			for time_duo in day:
				# set first clock to 8:30am if before
				if time_duo[0] < datetime(hour=8, minute=30, year=1900, month=1, day=1):
					time_duo[0] = datetime(hour=8, minute=30, year=1900, month=1, day=1)
				
				overtime_today = False
				if self.overtime_privilege:
					overtime_today = overtime_employees[self.name][day_index]

				# set last duo to end at 5pm unless employee has overtime privilege
				# else remove duos after 7 unless overtime till seven not on
				if not overtime_today and time_duo[1] > datetime(hour=17, minute=0, year=1900, month=1, day=1):
					time_duo[1] = datetime(hour=17, minute=0, year=1900, month=1, day=1)
				elif overtime_today and time_duo[1] > datetime(hour=19, minute=0, year=1900, month=1, day=1) and overtime_till_seven:
					time_duo[1] = datetime(hour=19, minute=0, year=1900, month=1, day=1)

		# calculate total hours and process hours for lunch
		for index, day in enumerate(self.time_duos):
			daily_total = timedelta(0)
			break_time = Employee.calc_break_time(day)
			for time_duo in day:
				daily_total += Employee.calc_length(time_duo)
			if len(day) == 1 and daily_total >= timedelta(hours=6):
				daily_total -= timedelta(minutes=30)
				break_time += timedelta(minutes=30)
			elif len(day) > 1 and daily_total >= timedelta(hours=6):
				if break_time < timedelta(minutes=30):
					daily_total -= (timedelta(minutes=30) - break_time)
					break_time = timedelta(minutes=30)

			# handle extra break time if true
			if extra_break and day:
				# if first clock in is before 8:45 and daily time is greater or equal to 7:45
				if day[0][0] < datetime(year=1900, month=1, day=1, hour=8, minute=45) and daily_total >= timedelta(hours=7, minutes=45):
					# calculate 8:45 - first clock
					minutes_clocked_in_before_eight_forty_five = datetime(year=1900, month=1, day=1, hour=8, minute=45) - day[0][0]
					print(self.name)
					
					print("minutes before 8:45: " + str(minutes_clocked_in_before_eight_forty_five))
					
					minutes_over_thirty_during_break = break_time - timedelta(minutes=30)
					print("minutes over thirty during break " + str(minutes_over_thirty_during_break))
					
					if not self.overtime_privilege:
						minutes_clocked_out_before_five_pm = datetime(year=1900, month=1, day=1, hour=17, minute=0) - day[-1][1]
					else:
						minutes_clocked_out_before_five_pm = timedelta(0)
					print("minutes clocked out before five pm: " + str(minutes_clocked_out_before_five_pm))
					
					extra_time = min(minutes_clocked_in_before_eight_forty_five, \
										(timedelta(minutes=15) - minutes_clocked_in_before_eight_forty_five) + minutes_over_thirty_during_break + minutes_clocked_out_before_five_pm)
					print("extra_time: " + str(extra_time))
					
					daily_total += extra_time
						
					# if no overtime and daily total is greater than 8 hours shorten
					# if not self.overtime_privilege and daily_total > timedelta(hours=8):
					#	extra_time = extra_time - (daily_total - timedelta(hours=8))
					#	daily_total = timedelta(hours=8)
					
					# add to all extra time
					self.extra_time += extra_time

			# if time more than or equal to than 7:45 round to 8:00
			if daily_total >= timedelta(hours=7, minutes=45) and daily_total < timedelta(hours=8, minutes=0):
				daily_total = timedelta(hours=8, minutes=0)
			
			self.daily_totals.append(daily_total)

			self.worked_time += daily_total

		# set other hours
		for row in init_array:
			if row[0] == "Sick Leave":
				self.sick_time = timedelta(hours=int(row[1]), minutes=int(row[2]))
			if row[0] == "Holiday Pay":
				self.holiday_time += timedelta(hours=int(row[1]), minutes=int(row[2]))
			if len(row) == 9:
				for vacation_string in row:
					if len(vacation_string) == 8 and vacation_string[0:2] == "VT":
						[hours, minutes] = vacation_string[3:].split(":")
						self.vacation_time += timedelta(hours=int(hours), minutes=int(minutes))
					elif len(vacation_string) == 9 and vacation_string[0:3] == "PVT":
						[hours, minutes] = vacation_string[4:].split(":")
						self.vacation_time += timedelta(hours=int(hours), minutes=int(minutes))

		# set overtime if worked more that 40hours
		if self.worked_time > timedelta(hours=40):
			self.overtime = self.worked_time - timedelta(hours=40)
			self.worked_time = timedelta(hours=40)

		self.total_time = self.worked_time + self.vacation_time + self.sick_time

		if self.overtime_privilege:
			self.total_time += self.overtime

	def list_time(self):
		print("**************************************")
		print(self.name)
		print("-----------------")
		print("overtime privilege: ", self.overtime_privilege)
		print("worked time: ", self.worked_time)
		print("sick time: ", self.sick_time)
		print("vacation time: ", self.vacation_time)
		print("extra time: ", self.extra_time)
		print("overtime: ", self.overtime)
		print("total time: ", self.total_time)
		print("-----------------")
		for index, date in enumerate(self.time_duos):
			if date:
				print(self.day_dict[index])
				for time in date:
					print(time)
				print(self.daily_totals[index])
				print("-----------------")

	def fix_day(self, day_index):
		input_validation = False

		# print(self.day_dict[day_index], "for employee ", self.name, " has an odd number of entries!")
		# print("The last entry is at ", self.times[day_index][-1].time(), ".")
		# print("Enter a time to clock this employee out. (note: Should be after the previous entry. Use format 15:23.)")

		while input_validation == False:
			# input_time = str(input("# "))
			text, ok = QInputDialog.getText(None, 'Odd Entries', self.day_dict[day_index] + ' for employee ' + self.name \
				+ " has an odd number of entries! \nThe last entry is at " + str(self.times[day_index][-1].time()) + \
				 ". \nEnter a time to clock this employee out. (note: Should be after the previous entry. Use format 15:23.)")
			if not ok:
				sys.exit()
			input_time = str(text)
			try:
				converted_time = datetime.strptime(input_time, '%H:%M')
				if converted_time > self.times[day_index][-1]:
					input_validation = True
				else:
					QMessageBox.warning(None, "Warning", "Invalid input. Try again.")
			except:
				QMessageBox.warning(None, "Warning", "Invalid input. Try again.")
		
		self.times[day_index].append(converted_time)

	@staticmethod
	def calc_break_time(day):
		break_time = timedelta(0)
		for index, time_duo in enumerate(day):
			if (index + 1) != len(day):
				break_time += day[index + 1][0] - day[index][1]
		return break_time

	@staticmethod
	def calc_length(time_duo):
		return time_duo[1] - time_duo[0]

class Preferences(QWidget):
	def __init__(self,parent= None):
		super().__init__()

		title = QLabel("Employees with overtime privileges:")

		self.settings = QSettings()
		self.local_names = {}

		self.lst = QListWidget()

		self.lst.itemClicked.connect(self.current_list_action)

		if self.settings.value("timecardProcessor/overtimeEmployees"):
			self.lst.addItems([str(i) for i in self.settings.value("timecardProcessor/overtimeEmployees")])
			self.local_names = self.settings.value("timecardProcessor/overtimeEmployees")

		add = QPushButton('Add', clicked=self.add_action)
		clear = QPushButton('Clear', clicked=self.clear_action)

		grid = QGridLayout(self)
		grid.setSpacing(10)

		grid.addWidget(title, 1, 1, 1, 2)
		grid.addWidget(self.lst, 2, 1, 1, 2)


		if os.path.exists("employees.txt"):
			with open("employees.txt", "r") as f:
				self.name_input = QComboBox()
				for x in f:
					self.name_input.addItem(x.rstrip())
				if self.name_input.count() == 0:
					self.name_input.setDisabled(True)
				else:
					self.name_input.setDisabled(False)
		else:
			self.name_input = QLineEdit()
			self.name_input.returnPressed.connect(self.add_action)
			self.name_input.setPlaceholderText("Doe, John")
			regexp = QRegExp('([A-Za-z])+(\ [A-Za-z]+)*\, ([A-Za-z])+(\ [A-Za-z]+)*')
			validator = QRegExpValidator(regexp)
			self.name_input.setValidator(validator)

		grid.addWidget(self.name_input, 3, 1, 1, 2)
		grid.addWidget(add, 4, 1)
		grid.addWidget(clear, 4, 2)

		line = QFrame()
		line.setGeometry(QRect())
		line.setFrameShape(QFrame.HLine)
		line.setFrameShadow(QFrame.Sunken)

		grid.addWidget(line, 5, 1, 1, 2)

		self.extra_time_checkbox = QCheckBox('Enable extra 15 minutes at lunch policy?')
		self.extra_time_checkbox.setChecked(self.settings.value("timecardProcessor/extraBreak", False, type=bool))
		self.extra_time_checkbox.clicked.connect(self.extra_time_action)

		grid.addWidget(self.extra_time_checkbox, 6, 1, 1, 2)

		self.sunday = QCheckBox('S')
		self.monday = QCheckBox('M')
		self.tuesday = QCheckBox('T')
		self.wednesday = QCheckBox('W')
		self.thursday = QCheckBox('T')
		self.friday = QCheckBox('F')
		self.saturday = QCheckBox('S')

		self.sunday.setDisabled(True)
		self.monday.setDisabled(True)
		self.tuesday.setDisabled(True)
		self.wednesday.setDisabled(True)
		self.thursday.setDisabled(True)
		self.friday.setDisabled(True)
		self.saturday.setDisabled(True)

		self.sunday.clicked.connect(lambda: self.overtime_day_action(3))
		self.monday.clicked.connect(lambda: self.overtime_day_action(4))
		self.tuesday.clicked.connect(lambda: self.overtime_day_action(5))
		self.wednesday.clicked.connect(lambda: self.overtime_day_action(6))
		self.thursday.clicked.connect(lambda: self.overtime_day_action(0))
		self.friday.clicked.connect(lambda: self.overtime_day_action(1))
		self.saturday.clicked.connect(lambda: self.overtime_day_action(2))

		self.sunday.day = 3
		self.monday.day = 4
		self.tuesday.day = 5
		self.wednesday.day = 6
		self.thursday.day = 0
		self.friday.day = 1
		self.saturday.day = 2

		num = QGridLayout()

		num.addWidget(self.sunday, 7, 1)
		num.addWidget(self.monday, 7, 2)
		num.addWidget(self.tuesday, 7, 3)
		num.addWidget(self.wednesday, 7, 4)
		num.addWidget(self.thursday, 7, 5)
		num.addWidget(self.friday, 7, 6)
		num.addWidget(self.saturday, 7, 7)

		grid.addLayout(num, 7, 1, 1, 2)
		
		self.overtime_seven_checkbox = QCheckBox('End overtime at 7 PM?')
		self.overtime_seven_checkbox.setChecked(self.settings.value("timecardProcessor/overtimeTillSeven", True, type=bool))
		self.overtime_seven_checkbox.clicked.connect(self.overtime_seven_action)
		grid.addWidget(self.overtime_seven_checkbox, 8, 1, 1, 2)
		
		grid.addWidget(line, 9, 1, 1, 2)
		
		versionWidget = QLabel(version)
		grid.addWidget(versionWidget, 10, 1, 1, 2)

	def add_action(self):
		if os.path.exists("employees.txt"):
			name_input_str = self.name_input.currentText().title()
		else:
			name_input_str = self.name_input.text().title()
		if name_input_str:
			it = QListWidgetItem(name_input_str)
			self.lst.addItem(it)
			self.local_names[name_input_str] = [1,1,1,1,1,1,1]
			self.settings.setValue("timecardProcessor/overtimeEmployees", self.local_names)
			self.lst.scrollToItem(it)
		if not os.path.exists("employees.txt"):
			self.name_input.setText("")
		else:
			self.name_input.removeItem(self.name_input.currentIndex())
			if self.name_input.count() == 0:
							self.name_input.setDisabled(True)

	def clear_action(self):
		self.lst.clear()
		self.local_names = {}
		self.settings.setValue("timecardProcessor/overtimeEmployees", {})

		self.name_input.clear()
		if os.path.exists("employees.txt"):
			with open("employees.txt", "r") as f:
				for x in f:
					self.name_input.addItem(x.rstrip())
				if self.name_input.count() == 0:
					self.name_input.setDisabled(True)
				else:
					self.name_input.setDisabled(False)

		self.sunday.setDisabled(True)
		self.monday.setDisabled(True)
		self.tuesday.setDisabled(True)
		self.wednesday.setDisabled(True)
		self.thursday.setDisabled(True)
		self.friday.setDisabled(True)
		self.saturday.setDisabled(True)

		self.sunday.setChecked(False)
		self.monday.setChecked(False)
		self.tuesday.setChecked(False)
		self.wednesday.setChecked(False)
		self.thursday.setChecked(False)
		self.friday.setChecked(False)
		self.saturday.setChecked(False)

	def extra_time_action(self):
		self.settings.setValue("timecardProcessor/extraBreak", self.extra_time_checkbox.isChecked())

	def overtime_day_action(self, day):
		c = self.local_names[self.lst.currentItem().text()][day]

		if c == 0:
			self.local_names[self.lst.currentItem().text()][day] = 1
		else:
			self.local_names[self.lst.currentItem().text()][day] = 0

		self.settings.setValue("timecardProcessor/overtimeEmployees", self.local_names)

		self.current_list_action()
		
	def overtime_seven_action(self):
		self.settings.setValue("timecardProcessor/overtimeTillSeven", self.overtime_seven_checkbox.isChecked())

	def current_list_action(self):
		self.sunday.setDisabled(False)
		self.monday.setDisabled(False)
		self.tuesday.setDisabled(False)
		self.wednesday.setDisabled(False)
		self.thursday.setDisabled(False)
		self.friday.setDisabled(False)
		self.saturday.setDisabled(False)

		self.sunday.setChecked(self.local_names[self.lst.currentItem().text()][3])
		self.monday.setChecked(self.local_names[self.lst.currentItem().text()][4])
		self.tuesday.setChecked(self.local_names[self.lst.currentItem().text()][5])
		self.wednesday.setChecked(self.local_names[self.lst.currentItem().text()][6])
		self.thursday.setChecked(self.local_names[self.lst.currentItem().text()][0])
		self.friday.setChecked(self.local_names[self.lst.currentItem().text()][1])
		self.saturday.setChecked(self.local_names[self.lst.currentItem().text()][2])

class App(QWidget):
	def __init__(self):
		super().__init__()
		
		self.employees_raw = []
		self.employee_current = []
		self.employees = []

		self.current_input = QSettings().value("timecardProcessor/currentInput")
		self.current_output = QSettings().value("timecardProcessor/currentOutput")

		self.initUI()
		
	def initUI(self):
		self.layout = QVBoxLayout()
		
		self.setGeometry(300, 300, 230, 100)
		self.setWindowTitle('Timecard Processor')
		self.setWindowIcon(QIcon('web.png'))   

		preferences_button = QPushButton("Preferences", self)
		preferences_button.clicked.connect(self.show_preferences_dialog)

		self.file_button = QPushButton("Input: " + self.current_input if self.current_input else "Open input...", self)
		self.file_button.clicked.connect(self.show_file_dialog)

		self.folder_button = QPushButton("Output: " + self.current_output if self.current_output else "Save output as...", self)
		self.folder_button.clicked.connect(self.show_folder_dialog)

		self.process_button = QPushButton("Process", self)
		self.process_button.setEnabled(bool(self.current_input and self.current_output))
		self.process_button.clicked.connect(self.process)

		self.layout.addWidget(preferences_button)
		self.layout.addWidget(self.file_button)
		self.layout.addWidget(self.folder_button)
		self.layout.addWidget(self.process_button)

		self.setLayout(self.layout)
	
		self.show()

		self.setFixedHeight(self.height())

	def process(self):
		self.process_button.setEnabled(False)
		self.process_button.setText("Working...")

		overtime_employees = QSettings().value("timecardProcessor/overtimeEmployees", {})
		extra_break = QSettings().value("timecardProcessor/extraBreak", False, type=bool)
		overtime_till_seven = QSettings().value("timecardProcessor/overtimeTillSeven", True, type=bool)

		with open(self.current_input) as csvfile:
			csvreader = csv.reader(csvfile, skipinitialspace=True, \
				delimiter=",", quoting=csv.QUOTE_NONE)
			
			past_row_dashes = False
			
			for row in csvreader:
				if any(field.strip() for field in row):
					if row[0] == "-----------------------------------------":
						self.employees_raw.append(self.employee_current)
						self.employee_current = []
						past_row_dashes = True
					elif (row[0] == "monday" or row[0] == "tuesday" or row[0] == "wednesday" or \
							row[0] == "thursday" or row[0] == "friday" or row[0] == "saturday" or \
							row[0] == "sunday") and past_row_dashes:
						self.employee_current = []
						past_row_dashes = False
					else:
						self.employee_current.append(row)
						past_row_dashes = False

		for employee_data in self.employees_raw:
			self.employees.append(Employee(employee_data, overtime_employees, extra_break, overtime_till_seven))

		for employee in self.employees:
			employee.list_time()

		self.write_csv()

		self.process_button.setText("Done! Open file?")
		self.process_button.setEnabled(True)

		self.employees_raw = []
		self.employee_current = []
		self.employees = []

		self.process_button.clicked.disconnect()
		self.process_button.clicked.connect(self.open_file)

	def show_preferences_dialog(self):
		self.preferences = Preferences()
		self.preferences.setWindowTitle('Preferences')
		self.preferences.setWindowModality(Qt.ApplicationModal)
		self.preferences.show()
		self.reset_process_button()

	def show_file_dialog(self):
		self.reset_process_button()

		fname = QFileDialog.getOpenFileName(self, 'Open', '/home', "csv(*.csv)")
		self.current_input = fname[0]
		QSettings().setValue("timecardProcessor/currentInput", fname[0])
		self.file_button.setText("Input: " + fname[0] if fname[0] else "Open input...")

		self.reset_process_button()

	def show_folder_dialog(self):
		self.reset_process_button()

		fname = QFileDialog.getSaveFileName(self, "Save output as...", "output", "csv(*.csv)")
		self.current_output = fname[0]
		QSettings().setValue("timecardProcessor/currentOutput", fname[0])
		self.folder_button.setText("Output: " + fname[0] if fname[0] else "Save as...")

		self.reset_process_button()

	def reset_process_button(self):
		if self.current_input != "" and self.current_output != "":
			self.process_button.setEnabled(True)
		else:
			self.process_button.setEnabled(False)

		self.process_button.setText("Process")
		self.process_button.clicked.disconnect()
		self.process_button.clicked.connect(self.process)

	def open_file(self):
		if os.name == "posix":
			subprocess.run(['open', self.current_output], check=True)
		else:
			os.startfile(self.current_output)

	def write_csv(self):
		with open(self.current_output, mode='w') as outputfile:
			csvwriter = csv.writer(outputfile, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
			csvwriter.writerow(["", "Employee's name", "CyberPay", "Holiday", "Sick", "Vacation", "Overtime", "Total"])

			totals = ["Totals", "", 0, 0, 0, 0, 0, 0]

			for index, employee in enumerate(self.employees):
				worked_seconds = employee.worked_time.seconds

				hours = employee.worked_time.days * 24 + employee.worked_time.seconds // 3600
				minutes = (employee.worked_time.seconds // 60) % 60

				# calculate row
				calc = [index + 1,
						("*" if employee.overtime_privilege else "") + employee.name,
						hours + round(minutes / 60, 2),
						round(employee.holiday_time.days * 24 + employee.holiday_time.seconds / 3600, 2),
						round(employee.sick_time.days * 24 + employee.sick_time.seconds / 3600, 2),
						round(employee.vacation_time.days * 24 + employee.vacation_time.seconds / 3600, 2),
						round(employee.overtime.days * 24 + employee.overtime.seconds / 3600, 2),
						round(employee.total_time.days * 24 + employee.total_time.seconds / 3600, 2)]

				calc_strings = []
				for number in calc:
					temp = ""
					if number == 0:
						calc_strings.append("")
					else:
						calc_strings.append(number)

				csvwriter.writerow(calc_strings)

				# add row to totals						
				totals[2] += calc[2]
				totals[3] += calc[3]
				totals[4] += calc[4]
				totals[5] += calc[5]
				totals[6] += calc[6]
				totals[7] += calc[7]

			strings = ["Totals", ""]

			# truncate totals
			for index, i in enumerate(totals[2:]):
				strings.append("%.2f" % i)

			csvwriter.writerow(strings)

if __name__ == '__main__':
	QCoreApplication.setApplicationName("Anton Zakharov")
	QCoreApplication.setOrganizationDomain("https://zazant.com")
	QCoreApplication.setApplicationName("Timecard Processor")

	appctxt = ApplicationContext()

	app = QApplication(sys.argv)
	ex = App()
	
	exit_code = appctxt.app.exec_()
	sys.exit(exit_code)
