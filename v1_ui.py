import tkinter as tk
from tkinter import ttk
import os
from functools import partial
from node import Node
import json
path = ""
current_path = ""
files = []
booted = False
ASSET_TYPES = {
    ".obj": "assets/models",
    ".fbx": "assets/models",
    ".gltf": "assets/models",
    ".glb": "assets/models",

    ".png": "assets/textures",
    ".pj": "assets/textures",
    ".jpg": "assets/textures",
    ".jpeg": "assets/textures",
    ".bmp": "assets/textures",
    ".tga": "assets/textures",

    ".wav": "assets/audio",
    ".ogg": "assets/audio",
    ".mp3": "assets/audio",

    ".mat": "assets/materials",

    ".scene": "scenes",

    ".py": "scripts",
}
import shutil
from tkinterdnd2 import DND_FILES, TkinterDnD
class Transform:
    def __init__(self):
        self.position = [0, 0, 0]
        self.rotation = [0, 0, 0]
        self.scale = [1, 1, 1]


class Node:
    uid_counter = 0
    
    def __init__(self, name="Node", node_type="Node"):
        self.name = name
        self.type = node_type

        self.transform = Transform()

        self.children = []
        self.parent = None

        self.uid = Node.uid_counter
        Node.uid_counter += 1
    def rename(nn):
        self.name=nn
    def add_child(self, node):
        node.parent = self
        self.children.append(node)
    @staticmethod
    def from_dict(parent,data):
        node=Node()
        node.parent=parent
        node.name=data["name"]
        transform=Transform()
        transform.position=data["transform"]["pos"]
        transform.rotation=data["transform"]["rot"]
        transform.scale=data["transform"]["scale"]
        node.transform=transform
        node.uid=data["uid"]
        Node.uid_counter=max(Node.uid_counter, data["uid"])
        child=data["children"]
        for c in child:
            childe=Node().from_dict(node,c)
            node.children.append(childe)
        return node
    def remove_child(self, node):
        if node in self.children:
            self.children.remove(node)
            node.parent = None


    def get_children(self):
        return self.children


    def get_parent(self):
        return self.parent
    def get_child(self,idx):
        return self.children[idx]
    def to_dict(self):
        """parent=self.parent
        if parent!=None:
            parent=self.parent.to_dict()"""
        return {"name":self.name, "type":self.type, "transform":{"pos":self.transform.position,"rot":self.transform.rotation, "scale":self.transform.scale}, "uid":self.uid, "children": [child.to_dict() for child in self.children]}

class Scene:
    def __init__(self, name="NewScene"):
        self.name = name
        self.root = Node("Root", "SceneRoot")

def show_files(folder_path):
    try:
        return os.listdir(folder_path)
    except:
        return []


def change_folder(folder, refresh_callback):
    global current_path, files

    if folder == "..":
        parent = os.path.dirname(current_path)

        if parent.startswith(path):
            current_path = parent

    else:
        new_path = os.path.join(current_path, folder)

        if os.path.isdir(new_path):
            current_path = new_path
        else:
            print("Opening file:", new_path)
            return

    files = show_files(current_path)
    refresh_callback()
    
def start(project_path):
    global path, current_path, files, booted, scenes, curent_scene

    path = project_path
    current_path = project_path

    if not booted:
        files = show_files(current_path)
        booted = True
    
    root = TkinterDnD.Tk()
    root.title("VOXEL_GE")
    root.geometry("1200x750")
    root.configure(bg="#181a1f")
    
    root.in_3d_view = True
    current_scene = Scene("NewScene")
    selected_node = current_scene.root
    current_scene.name=current_scene.root.name
    def read_scene(event=None):
        read_path=project_path+"/scenes/"+current_scene.name+".json"
        try:
            with open(read_path, "r") as f:
                current_scene.root=Node.from_dict(None,json.load(f))
        except Exception:
            return
        finally:
            return
    def write_scene(event=None):
        save_path=project_path+"/scenes/"+current_scene.name+".json"
        with open(save_path, "w") as f:
            json.dump(current_scene.root.to_dict(), f,indent=4)
        print(current_scene.root.get_children())
    read_scene()
    root.bind("<Control-s>", write_scene)
    BG = "#181a1f"
    PANEL = "#252932"
    HEADER = "#303641"
    TEXT = "#dddddd"
    ACCENT = "#62b0ff"
    style = ttk.Style()
    style.theme_use("clam")

    style.configure(
        "TButton",
        background=HEADER,
        foreground=TEXT,
        borderwidth=0
    )


    toolbar = tk.Frame(root, bg=HEADER, height=40)
    toolbar.pack(fill="x")


    tk.Label(
        toolbar,
        text="VOXEL_GE",
        bg=HEADER,
        fg=TEXT,
        font=("Arial", 12, "bold")
    ).pack(side="left", padx=15)


    view_mode = tk.StringVar(value="SCRIPT")


    def toggle_view():
        if root.in_3d_view:
            view_mode.set("3D")
            viewport.config(text="Script Workspace")
            root.in_3d_view = False
        else:
            view_mode.set("SCRIPT")
            viewport.config(text="Voxel Viewport")
            root.in_3d_view = True


    ttk.Button(
        toolbar,
        textvariable=view_mode,
        command=toggle_view
    ).pack(side="right", padx=10)



    main = tk.Frame(root, bg=BG)
    main.pack(fill="both", expand=True)

    scene_panel = tk.Frame(
        main,
        bg=PANEL,
        width=220
    )

    scene_panel.pack(
        side="left",
        fill="y"
    )


    tk.Label(
        scene_panel,
        text="SCENE",
        bg=PANEL,
        fg=ACCENT
    ).pack(anchor="w", padx=10, pady=8)
    scene_tree = tk.Frame(
        scene_panel,
        bg=PANEL,
    )
    
    def add_node():
        global selected_node

        new_node = Node(
            name="NewNode",
            node_type="Node"
        )

        selected_node.add_child(new_node)

        refresh_scene_tree()
    add_button = tk.Button(
        scene_panel,
        text="+ Add Node",
        bg="#303641",
        fg=TEXT,
        command=lambda: add_node()
    )

    add_button.pack(
        fill="x",
        padx=10,
        pady=5
    )
    

    scene_tree.pack(fill="both", expand=True)


    def refresh_scene_tree():

        for widget in scene_tree.winfo_children():
            widget.destroy()


        def add_node_button(node, depth=0):

            tk.Button(
                scene_tree,
                text=("  " * depth) + node.name,
                bg=PANEL,
                fg=TEXT,
                anchor="w",
                command=lambda n=node: select_node(n)
            ).pack(
                fill="x"
            )


            for child in node.children:
                add_node_button(child, depth + 1)


        add_node_button(current_scene.root)



    def select_node(node):
        global selected_node

        selected_node = node
        update_inspector()



    refresh_scene_tree()
    def rename_node(event):
        global selected_node
        node=None
        for widget in scene_tree.winfo_children():
            xc=widget.winfo_x()-widget.winfo_width()/2 < event.x < widget.winfo_x()+widget.winfo_width()/2
            yc=widget.winfo_y()-widget.winfo_height()/2 < event.y < widget.winfo_y()+widget.winfo_height()/2
            if xc and yc:
                node=widget
                break
        if node==None:
            return
        x,y,w,h=node.winfo_x(), node.winfo_y(), node.winfo_width(), node.winfo_height()
        old_name=selected_node.name
        entry=tk.Entry(root)
        entry.insert(0,old_name)
        entry.select_range(0,tk.END)
        entry.focus()
        entry.place(x=x,y=y,width=w,height=h)
        def finish(event):
            new_name=entry.get()
            selected_node.name=new_name
            entry.destroy()
            refresh_scene_tree()
        entry.bind("<Return>",finish)
        entry.bind("<FocusOut>",finish)
        #selected_node.name="renamed"
        
    root.bind("<Double-1>",rename_node)


    viewport = tk.Label(
        main,
        text="Voxel Viewport",
        bg="#101216",
        fg=TEXT,
        font=("Arial", 24)
    )

    viewport.pack(
        side="left",
        fill="both",
        expand=True
    )



    right = tk.Frame(
        main,
        bg=PANEL,
        width=280
    )

    right.pack(
        side="right",
        fill="y"
    )



    inspector = tk.Frame(
        right,
        bg=PANEL,
        height=300
    )

    inspector.pack(fill="x")


    tk.Label(
        inspector,
        text="INSPECTOR",
        bg=PANEL,
        fg=ACCENT
    ).pack(anchor="w", padx=10, pady=8)


    inspector_data = tk.Frame(
        inspector,
        bg=PANEL
    )

    inspector_data.pack(
        fill="both"
    )


    def update_inspector():

        for widget in inspector_data.winfo_children():
            widget.destroy()


        tk.Label(
            inspector_data,
            text="NAME: " + selected_node.name,
            bg=PANEL,
            fg=TEXT
        ).pack(anchor="w", padx=15)


        tk.Label(
            inspector_data,
            text="TYPE: " + selected_node.type,
            bg=PANEL,
            fg=TEXT
        ).pack(anchor="w", padx=15)


        tk.Label(
            inspector_data,
            text="TRANSFORM",
            bg=PANEL,
            fg=ACCENT
        ).pack(anchor="w", padx=15, pady=5)


        tk.Label(
            inspector_data,
            text="Position: " + str(selected_node.transform.position),
            bg=PANEL,
            fg=TEXT
        ).pack(anchor="w", padx=15)


        tk.Label(
            inspector_data,
            text="Rotation: " + str(selected_node.transform.rotation),
            bg=PANEL,
            fg=TEXT
        ).pack(anchor="w", padx=15)


        tk.Label(
            inspector_data,
            text="Scale: " + str(selected_node.transform.scale),
            bg=PANEL,
            fg=TEXT
        ).pack(anchor="w", padx=15)


    update_inspector()



    filesystem = tk.Frame(
        right,
        bg="#202329"
    )

    filesystem.pack(
        fill="both",
        expand=True
    )
    

    tk.Label(
        filesystem,
        text="FILESYSTEM",
        bg="#202329",
        fg=ACCENT
    ).pack(
        anchor="w",
        padx=10,
        pady=8
    )

    def on_drop(event):

        filepath = event.data.strip("{}")

        ext = os.path.splitext(filepath)[1].lower()

        if ext in ASSET_TYPES:

            destination = os.path.join(
                path,
                ASSET_TYPES[ext]
            )

            os.makedirs(destination, exist_ok=True)

            shutil.copy2(
                filepath,
                destination
            )

            refresh_files()
    filesystem.drop_target_register(DND_FILES)
    filesystem.dnd_bind("<<Drop>>", on_drop)
    file_area = tk.Frame(
        filesystem,
        bg="#202329"
    )

    file_area.pack(
        fill="both",
        expand=True
    )


    file_area = tk.Frame(
        filesystem,
        bg="#202329"
    )

    file_area.pack(
        fill="both",
        expand=True
    )


    def refresh_files():

        for widget in file_area.winfo_children():
            widget.destroy()

        if current_path != path:
            tk.Button(
                file_area,
                text="..",
                bg="#202329",
                fg=TEXT,
                command=partial(change_folder, "..", refresh_files)
            ).pack(
                anchor="w",
                padx=15
            )


        for f in files:
            tk.Button(
                file_area,
                text=f,
                bg="#202329",
                fg=TEXT,
                command=partial(change_folder, f, refresh_files)
            ).pack(
                anchor="w",
                padx=15
            )


    refresh_files()




    output = tk.Frame(
        root,
        bg=HEADER,
        height=100
    )

    output.pack(fill="x")


    tk.Label(
        output,
        text="OUTPUT",
        bg=HEADER,
        fg=ACCENT
    ).pack(
        anchor="w",
        padx=10
    )


    console = tk.Text(
        output,
        height=4,
        bg="#101216",
        fg=TEXT
    )

    console.pack(
        fill="x",
        padx=10
    )


    console.insert(
        "end",
        "VOXEL_GE initialized...\n"
    )


    root.mainloop()



if __name__ == "__main__":
    start("C:/eg_path")
