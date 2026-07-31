import tkinter as tk
from new_project import NewProjectWindow
root = tk.Tk()

def open_new_project():
    NewProjectWindow(root)

tk.Button(
    root,
    text="New Project",
    command=open_new_project
).pack()
root.mainloop()
