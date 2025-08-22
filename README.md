# day-5
Student-Management-System (Final)

A **menu-driven C program** that works as a complete **Student Management System** for schools and colleges.  
This is the final version of the Day-wise project series.

---

## Features
- Add, View, Update, and Delete (CRUD) student records  
- Accept multiple subject marks per student  
- Input validation (marks between 0–100)  
- Automatic average calculation and grade assignment (A–F)  
- Persistent storage using `students.txt` (data saved across runs)  
- Search students by **ID** or **Name**  
- Sort students by **Name**, **ID**, or **Average Marks**  
- Generate a clean **Student Report** with all details  
- Admin login system for restricted access  
- User-friendly CLI interface

---

## How It Works
1. On startup, the program loads existing student records from `students.txt`  
2. User logs in with admin credentials  
3. Menu options allow performing CRUD operations, searching, sorting, and generating reports  
4. Data is saved automatically back into the file  

---

## Files
- `student_management_system_final.c` → Main source code  
gcc student_management_system_final.c -o student_management_system_final
./student_management_system_final
