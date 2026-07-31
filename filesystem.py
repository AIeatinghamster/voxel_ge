import os
import json
from asset_ import Asset
class VoxelFileSystem:

    def __init__(self, project_path):
        self.project_path = project_path


    def create_project(self, name):

        path = os.path.join(
            self.project_path,
            name
        )
        
        folders = [
            "assets",
            "scenes",
            "scripts",
            "assets/models",
            "assets/textures",
            "assets/materials",
            "assets/audio"
        ]

        os.makedirs(path, exist_ok=True)

        for folder in folders:
            os.makedirs(
                os.path.join(path, folder),
                exist_ok=True
            )


        project = {
            "name": name,
            "engine": "VOXEL_GE",
            "version": "0.1"
        }


        with open(
            os.path.join(path, "project.vge"),
            "w"
        ) as file:
            json.dump(
                project,
                file,
                indent=4
            )


        return path



    def list_files(self):

        result = []

        for root, dirs, files in os.walk(
            self.project_path
        ):

            for file in files:
                result.append(
                    os.path.join(
                        root,
                        file
                    )
                )

        return result



    def save_file(
        self,
        location,
        data
    ):

        path = os.path.join(
            self.project_path,
            location
        )

        with open(
            path,
            "w"
        ) as file:
            file.write(data)



    def load_file(
        self,
        location
    ):

        path = os.path.join(
            self.project_path,
            location
        )

        with open(
            path,
            "r"
        ) as file:

            return file.read()
