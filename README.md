FitTracker — Simple Fitness Guide (Windows)
===========================================

HOW TO RUN
----------
1. Keep ALL files in the SAME folder
2. Double-click FitTracker.exe  (no installation needed)

HOW IT WORKS
------------
1. SETUP SCREEN
   Pick your Gender and Goal (Fat Loss / Muscle Gain / Both), then
   click "Get Started". Your choice is saved automatically.

2. DASHBOARD
   Three large tiles:
   - Diet Plan     → shows your personalized 7-day meal plan
   - Workout Plan  → shows your personalized 7-day gym schedule
   - Tracker       → log and review your daily progress

3. TRACKER
   - Tick the checkbox if you completed your diet / workout today
   - Add notes (meals eaten, exercises done)
   - Enter calories consumed and burned
   - Click "Save Today's Log"
   - Your last 10 entries are shown in the history table below

   The tracker shows only the sections relevant to your goal:
     Fat Loss    → Diet section only
     Muscle Gain → Workout section only
     Both        → Diet AND Workout sections

DATA FILES
----------
  config.txt           Your saved gender + goal
  progress.csv         Your daily progress log
  diet_*.txt           7-day diet plan files (included)
  workout_*.txt        7-day workout plan files (included)

BUILD FROM SOURCE
-----------------
  gcc main_win32.c -o FitTracker.exe -mwindows -lcomctl32 -O2
