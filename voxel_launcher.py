import tkinter as tk
from tkinter import filedialog
from tkinter import messagebox
import os
import sys
import time
import json
from new_project import NewProjectWindow
import v1_ui
import subprocess

REGISTRY_FILE = "project_registry.json"



class ProjectManager:


    def __init__(self):

        self.registry = REGISTRY_FILE


        if not os.path.exists(
            self.registry
        ):
            self.save([])

    def remove_project(self, path):
        path = os.path.abspath(path)

        projects = self.load()

        projects = [
            project for project in projects
            if project["path"] != path
        ]

        self.save(projects)

    def save(self, projects):

        with open(
            self.registry,
            "w"
        ) as file:

            json.dump(
                projects,
                file,
                indent=4
            )



    def load(self):

        try:

            with open(
                self.registry
            ) as file:

                return json.load(file)

        except:

            return []



    def add_project(
        self,
        path
    ):
        
        projects = self.load()


        path = os.path.abspath(
            path
        )


        for project in projects:

            if project["path"] == path:

                return



        name = os.path.basename(
            path
        )


        project = {

            "name": name,

            "path": path,

            "thumbnail": "",

            "last_opened": "",

            "created":
                time.time()

        }


        projects.append(
            project
        )


        self.save(
            projects
        )



    def scan_folder(
        self,
        folder
    ):

        for root, dirs, files in os.walk(
            folder
        ):

            if "project.vge" in files:

                self.add_project(
                    root
                )



    def get_projects(self):

        projects = self.load()


        valid = []


        for project in projects:


            if os.path.exists(
                project["path"]
            ):

                valid.append(
                    project
                )


        self.save(
            valid
        )


        return valid





class VoxelLauncher:


    def __init__(self):

        self.root = tk.Tk()


        self.root.title(
            "VOXEL_GE Launcher"
        )


        self.root.geometry(
            "1000x650"
        )


        self.root.configure(
            bg="#181a1f"
        )


        self.manager = ProjectManager()
        
        self.create_ui()


        self.live_update()

        self.root.mainloop()



    def remove_project(self, project):

        if messagebox.askyesno(
            "Remove Project",
            f"Remove '{project['name']}' from the launcher?\n\n"
            "The project files will NOT be deleted."
        ):

            self.manager.remove_project(
                project["path"]
            )

            self.refresh_projects()

            self.status.config(
                text=f"Removed {project['name']}"
            )

    def create_ui(self):


        header = tk.Frame(
            self.root,
            bg="#252932",
            height=70
        )

        header.pack(
            fill="x"
        )


        tk.Label(
            header,
            text="VOXEL_GE",
            bg="#252932",
            fg="#62b0ff",
            font=(
                "Arial",
                24,
                "bold"
            )
        ).pack(
            side="left",
            padx=25
        )



        tk.Label(
            header,
            text="Voxel Game Engine",
            bg="#252932",
            fg="white"
        ).pack(
            side="left"
        )



        self.project_area = tk.Frame(
            self.root,
            bg="#181a1f"
        )

        self.project_area.pack(
            fill="both",
            expand=True,
            padx=25,
            pady=25
        )





        footer = tk.Frame(
            self.root,
            bg="#252932",
            height=60
        )

        footer.pack(
            fill="x"
        )



        tk.Button(
            footer,
            text="+ CREATE PROJECT",
            command=self.create_project,
            bg="#30343d",
            fg="white"
        ).pack(
            side="left",
            padx=15,
            pady=15
        )



        tk.Button(
            footer,
            text="SCAN FOLDER",
            command=self.scan_folder,
            bg="#30343d",
            fg="white"
        ).pack(
            side="left"
        )



        tk.Button(
            footer,
            text="IMPORT PROJECT",
            command=self.import_project,
            bg="#30343d",
            fg="white"
        ).pack(
            side="left",
            padx=15
        )



        self.status = tk.Label(
            footer,
            text="Ready",
            bg="#252932",
            fg="#888888"
        )

        self.status.pack(
            side="right",
            padx=20
        )

        


    def refresh_projects(self):


        for widget in self.project_area.winfo_children():

            widget.destroy()



        projects = (
            self.manager.get_projects()
        )


        if not projects:

            tk.Label(
                self.project_area,
                text="No VOXEL_GE projects found",
                bg="#181a1f",
                fg="white",
                font=(
                    "Arial",
                    16
                )
            ).pack(
                pady=60
            )

            return



        for project in projects:

            self.project_card(
                project
            )



    def project_card(
        self,
        project
    ):

        
        card = tk.Frame(
            self.project_area,
            bg="#252932",
            width=240,
            height=200
        )


        card.pack(
            side="left",
            padx=15,
            pady=15
        )


        card.pack_propagate(
            False
        )


        tk.Label(
            card,
            text="🧊",
            bg="#252932",
            fg="white",
            font=(
                "Arial",
                40
            )
        ).pack(
            pady=10
        )



        tk.Label(
            card,
            text=project["name"],
            bg="#252932",
            fg="white",
            font=(
                "Arial",
                12,
                "bold"
            )
        ).pack()



        tk.Label(
            card,
            text=project["path"],
            bg="#252932",
            fg="#888888",
            wraplength=210
        ).pack()



        button_frame = tk.Frame(
            card,
            bg="#252932"
        )
        button_frame.pack(pady=10)

        tk.Button(
            button_frame,
            text="OPEN",
            command=lambda p=project:
                self.open_project(p)
        ).pack(side="left", padx=5)

        tk.Button(
            button_frame,
            text="REMOVE",
            bg="#8b2c2c",
            fg="white",
            command=lambda p=project:
                self.remove_project(p)
        ).pack(side="left", padx=5)







    def live_update(self):


        self.refresh_projects()


        self.status.config(
            text="Live scanning"
        )


        self.root.after(
            2000,
            self.live_update
        )




    def create_project(self):

        NewProjectWindow(
            self.root
        )
        




    def scan_folder(self):

        folder = filedialog.askdirectory()


        if folder:

            self.manager.scan_folder(
                folder
            )


            self.refresh_projects()




    def import_project(self):

        folder = filedialog.askdirectory()


        if folder:

            if os.path.exists(
                os.path.join(
                    folder,
                    "project.vge"
                )
            ):

                self.manager.add_project(
                    folder
                )

                self.refresh_projects()



    def open_project(
        self,
        project
    ):

        project["last_opened"] = (
            time.time()
        )
        

        self.manager.save(
            self.manager.get_projects()
        )
        



        self.root.destroy()
        v1_ui.start(project["path"])
        sys.exit()
        





if __name__ == "__main__":

    VoxelLauncher()
