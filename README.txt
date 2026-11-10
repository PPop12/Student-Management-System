STUDENT MANAGEMENT SYSTEM - PBL

OVERVIEW
A small college student-record web application. The backend is written in C++ and
uses a singly linked list for in-memory records. Records are saved in students.csv.

FEATURES
- View all student records
- Add a student (roll number must be unique)
- Search by roll number or name
- Delete a student by roll number
- Sort by CGPA, name, or attendance
- Dashboard totals and averages
- CSV persistence (records remain after restarting the server)

REQUIREMENTS
- Windows
- MinGW g++ compiler
- A modern web browser

RUN
1. Extract the ZIP to a folder.
2. Open PowerShell or Command Prompt in that folder.
3. Compile:
   g++ server.cpp -std=c++17 -o server.exe -lws2_32
4. Start:
   .\server.exe
5. Open this in your browser:
   http://localhost:8080
6. Stop the server with Ctrl+C.

IMPORTANT
- Keep index.html, style.css, app.js and students.csv in the same folder as server.exe.
- The sample student data is synthetic and created for demonstration. It is not
  an official college dataset or real student information.
- The server is a local academic prototype, not a production deployment.
- If port 8080 is already in use, close the other application using it.
