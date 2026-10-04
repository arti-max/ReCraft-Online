#!/usr/bin/env python3
import gzip
import io
import math
import os
import struct
import threading
import tkinter as tk
from collections import deque
from tkinter import filedialog, messagebox, ttk

# Реестр блоков строго по Tile.cpp:
# ID: (Имя, HexColor, [top_tex, bottom_tex, side_tex])
TILES = {
    0:  ("Воздух (Air)", "#1A1A1A", [0, 0, 0]),
    1:  ("Камень (Rock)", "#757575", [1, 1, 1]),
    2:  ("Трава (Grass)", "#567D46", [0, 2, 3]),
    3:  ("Земля (Dirt)", "#866043", [2, 2, 2]),
    4:  ("Булыжник (Cobblestone)", "#616161", [16, 16, 16]),
    5:  ("Доски (Wood)", "#9C784E", [4, 4, 4]),
    6:  ("Куст (Bush)", "#3B6E22", [15, 15, 15]),
    7:  ("Бедрок (Bedrock)", "#2B2B2B", [17, 17, 17]),
    8:  ("Текущая вода (Water)", "#2E5299", [14, 14, 14]),
    9:  ("Спокойная вода (Calm Water)", "#26437D", [14, 14, 14]),
    10: ("Текущая лава (Lava)", "#C44601", [30, 30, 30]),
    11: ("Спокойная лава (Calm Lava)", "#A33A00", [30, 30, 30]),
    12: ("Гравий (Gravel)", "#7D7B7A", [19, 19, 19]),
    13: ("Песок (Sand)", "#D9CA84", [18, 18, 18]),
    14: ("Древесина (Log)", "#61472B", [21, 21, 20]),
    15: ("Листва (Leaves)", "#355E2B", [22, 22, 22]),
    16: ("Золотая руда (Gold Ore)", "#8F8969", [32, 32, 32]),
    17: ("Железная руда (Iron Ore)", "#8C7F78", [33, 33, 33]),
    18: ("Угольная руда (Coal Ore)", "#404040", [34, 34, 34]),
    19: ("Губка (Sponge)", "#C4BE4B", [48, 48, 48]),
    20: ("Стекло (Glass)", "#C2E6E8", [49, 49, 49]),
    21: ("Шерсть: Белая (Wool 1)", "#DEDEDE", [64, 64, 64]),
    22: ("Шерсть: Оранжевая (Wool 2)", "#D96814", [65, 65, 65]),
    23: ("Шерсть: Пурпурная (Wool 3)", "#B339B5", [66, 66, 66]),
    24: ("Шерсть: Голубая (Wool 4)", "#4A8AC9", [67, 67, 67]),
    25: ("Шерсть: Желтая (Wool 5)", "#D9B814", [68, 68, 68]),
    26: ("Шерсть: Лаймовая (Wool 6)", "#57B823", [69, 69, 69]),
    27: ("Шерсть: Розовая (Wool 7)", "#CC6281", [70, 70, 70]),
    28: ("Шерсть: Серая (Wool 8)", "#3D3D3D", [71, 71, 71]),
    29: ("Шерсть: Светло-серая (Wool 9)", "#8F8F8F", [72, 72, 72]),
    30: ("Шерсть: Бирюзовая (Wool 10)", "#1D7575", [73, 73, 73]),
    31: ("Шерсть: Фиолетовая (Wool 11)", "#6A229C", [74, 74, 74]),
    32: ("Шерсть: Синяя (Wool 12)", "#26329C", [75, 75, 75]),
    33: ("Шерсть: Коричневая (Wool 13)", "#4F321A", [76, 76, 76]),
    34: ("Шерсть: Зеленая (Wool 14)", "#354A18", [77, 77, 77]),
    35: ("Шерсть: Красная (Wool 15)", "#8A2020", [78, 78, 78]),
    36: ("Шерсть: Черная (Wool 16)", "#141414", [79, 79, 79]),
    37: ("Красный цветок (Red Flower)", "#B52424", [12, 12, 12]),
    38: ("Желтый цветок (Yellow Flower)", "#D9C727", [13, 13, 13]),
    39: ("Красный гриб (Red Mushroom)", "#BA2B2B", [28, 28, 28]),
    40: ("Коричневый гриб (Brown Mushroom)", "#876F5B", [29, 29, 29]),
    41: ("Золотой блок (Gold Block)", "#E6C829", [24, 24, 24]),
    42: ("Железный блок (Iron Block)", "#D4D4D4", [23, 23, 23]),
    43: ("Плита (Slab)", "#949494", [6, 6, 5]),
    44: ("Двойная плита (Double Slab)", "#949494", [6, 6, 5]),
    45: ("ТНТ (TNT)", "#BD3324", [9, 10, 8]),
    46: ("Кирпичи (Bricks)", "#853D31", [7, 7, 7]),
    47: ("Книжная полка (Bookshelf)", "#785633", [4, 4, 35]),
    48: ("Замшелый камень (Moss Stone)", "#547359", [36, 36, 36]),
    49: ("Паутина (Cobweb)", "#CCCCCC", [11, 11, 11]),
}

# Блоки-растения/паутина с крестообразной X-моделью
BUSH_BLOCKS = {6, 37, 38, 39, 40, 49}

# Жидкости
WATER_BLOCKS = {8, 9}
LAVA_BLOCKS = {10, 11}
LIQUID_BLOCKS = WATER_BLOCKS | LAVA_BLOCKS

# Прозрачные блоки (не блокирующие свет и видимость соседних граней)
TRANSPARENT_BLOCKS = {0, 6, 15, 20, 37, 38, 39, 40, 49}

CLIENT_MAGIC = 656127880
SERVER_MAGIC = 0xDEADBEEF


def hex_to_rgb(hex_str):
    hex_str = hex_str.lstrip("#")
    return tuple(int(hex_str[i : i + 2], 16) / 255.0 for i in (0, 2, 4))


class BinaryReader:
    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0

    def read_i8(self) -> int:
        val = struct.unpack_from(">b", self.data, self.offset)[0]
        self.offset += 1
        return val

    def read_u8(self) -> int:
        val = self.data[self.offset]
        self.offset += 1
        return val

    def read_bool(self) -> bool:
        return self.read_u8() == 1

    def read_i16(self) -> int:
        val = struct.unpack_from(">h", self.data, self.offset)[0]
        self.offset += 2
        return val

    def read_i32(self) -> int:
        val = struct.unpack_from(">i", self.data, self.offset)[0]
        self.offset += 4
        return val

    def read_i32_le(self) -> int:
        val = struct.unpack_from("<i", self.data, self.offset)[0]
        self.offset += 4
        return val

    def read_u32_le(self) -> int:
        val = struct.unpack_from("<I", self.data, self.offset)[0]
        self.offset += 4
        return val

    def read_i64(self) -> int:
        val = struct.unpack_from(">q", self.data, self.offset)[0]
        self.offset += 8
        return val

    def read_float(self) -> float:
        val = struct.unpack_from(">f", self.data, self.offset)[0]
        self.offset += 4
        return val

    def read_utf(self) -> str:
        length = self.read_i16()
        text = self.data[self.offset : self.offset + length].decode("utf-8", errors="replace")
        self.offset += length
        return text

    def read_bytes(self, n: int) -> bytearray:
        res = bytearray(self.data[self.offset : self.offset + n])
        self.offset += n
        return res


class BinaryWriter:
    def __init__(self):
        self.buf = bytearray()

    def write_u8(self, val: int):
        self.buf.append(val & 0xFF)

    def write_bool(self, val: bool):
        self.buf.append(1 if val else 0)

    def write_i16(self, val: int):
        self.buf.extend(struct.pack(">h", int(val)))

    def write_i32(self, val: int):
        self.buf.extend(struct.pack(">i", int(val)))

    def write_i64(self, val: int):
        self.buf.extend(struct.pack(">q", int(val)))

    def write_float(self, val: float):
        self.buf.extend(struct.pack(">f", float(val)))

    def write_utf(self, text: str):
        encoded = text.encode("utf-8")
        self.write_i16(len(encoded))
        self.buf.extend(encoded)

    def write_bytes(self, data: bytes | bytearray):
        self.buf.extend(data)


class LevelData:
    def __init__(self):
        self.name = "Level"
        self.creator = "Admin"
        self.creation_time = 0
        self.width = 64
        self.height = 64  # Z
        self.depth = 64   # Y (высота)
        self.blocks = bytearray(64 * 64 * 64)

        self.spawn_x = 32
        self.spawn_y = 32
        self.spawn_z = 32
        self.spawn_rot = 0

        self.health = 20
        self.air_supply = 300
        self.score = 0
        self.arrows = 0
        self.inv_slots = [-1] * 9
        self.inv_counts = [0] * 9
        self.entities = []

    def get_block(self, x: int, y: int, z: int) -> int:
        if 0 <= x < self.width and 0 <= y < self.depth and 0 <= z < self.height:
            return self.blocks[(y * self.height + z) * self.width + x]
        return 0

    def set_block(self, x: int, y: int, z: int, tile_id: int):
        if 0 <= x < self.width and 0 <= y < self.depth and 0 <= z < self.height:
            self.blocks[(y * self.height + z) * self.width + x] = tile_id & 0xFF

    def replace_blocks(self, old_id: int, new_id: int, target_y: int = None) -> int:
        count = 0
        if target_y is not None:
            if not (0 <= target_y < self.depth):
                return 0
            for z in range(self.height):
                for x in range(self.width):
                    idx = (target_y * self.height + z) * self.width + x
                    if self.blocks[idx] == old_id:
                        self.blocks[idx] = new_id
                        count += 1
        else:
            for i in range(len(self.blocks)):
                if self.blocks[i] == old_id:
                    self.blocks[i] = new_id
                    count += 1
        return count


# --- 3D OPENGL ВЬЮВЕР С ПОДДЕРЖКОЙ СТРОГОГО КАЛЛИНГА ЖИДКОСТЕЙ ---
def run_opengl_viewer(level: LevelData, terrain_path: str = ""):
    try:
        import numpy as np
        import pygame
        from OpenGL.GL import (
            GL_ALPHA_TEST,
            GL_BLEND,
            GL_CLAMP_TO_EDGE,
            GL_COLOR_ARRAY,
            GL_COLOR_BUFFER_BIT,
            GL_CULL_FACE,
            GL_DEPTH_BUFFER_BIT,
            GL_DEPTH_TEST,
            GL_FLOAT,
            GL_GREATER,
            GL_LEQUAL,
            GL_MODELVIEW,
            GL_MODULATE,
            GL_NEAREST,
            GL_ONE_MINUS_SRC_ALPHA,
            GL_PROJECTION,
            GL_QUADS,
            GL_RGBA,
            GL_SRC_ALPHA,
            GL_TEXTURE_2D,
            GL_TEXTURE_COORD_ARRAY,
            GL_TEXTURE_ENV,
            GL_TEXTURE_ENV_MODE,
            GL_TEXTURE_MAG_FILTER,
            GL_TEXTURE_MIN_FILTER,
            GL_TEXTURE_WRAP_S,
            GL_TEXTURE_WRAP_T,
            GL_UNSIGNED_BYTE,
            GL_VERTEX_ARRAY,
            glAlphaFunc,
            glBindTexture,
            glBlendFunc,
            glClear,
            glClearColor,
            glColorPointer,
            glDepthFunc,
            glDisable,
            glDisableClientState,
            glDrawArrays,
            glEnable,
            glEnableClientState,
            glGenTextures,
            glLoadIdentity,
            glMatrixMode,
            glRotatef,
            glTexCoordPointer,
            glTexEnvi,
            glTexImage2D,
            glTexParameteri,
            glTranslatef,
            glVertexPointer,
        )
        from OpenGL.GLU import gluPerspective
    except ImportError:
        messagebox.showerror(
            "Отсутствуют библиотеки",
            "Для 3D-рендеринга необходимы pygame, PyOpenGL и numpy.\nУстановите их:\npip install pygame PyOpenGL numpy --break-system-packages",
        )
        return

    pygame.init()
    screen_w, screen_h = 1000, 700
    pygame.display.set_mode((screen_w, screen_h), pygame.DOUBLEBUF | pygame.OPENGL)
    pygame.display.set_caption("ReCraft 3D Engine")
    pygame.mouse.set_visible(False)
    pygame.event.set_grab(True)

    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LEQUAL)

    # Альфа-тестирование для вырезания прозрачных фрагментов текстур листвы, цветов, грибов и паутины
    glEnable(GL_ALPHA_TEST)
    glAlphaFunc(GL_GREATER, 0.1)

    # Альфа-блендинг для полупрозрачного стекла
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

    glClearColor(0.55, 0.75, 0.95, 1.0)

    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(70, (screen_w / screen_h), 0.1, 1000.0)
    glMatrixMode(GL_MODELVIEW)

    # Загрузка текстуры terrain.png
    has_texture = False
    if terrain_path and os.path.isfile(terrain_path):
        try:
            surface = pygame.image.load(terrain_path).convert_alpha()
            img_data = pygame.image.tostring(surface, "RGBA", False)
            w_tex, h_tex = surface.get_size()

            tex_id = glGenTextures(1)
            glBindTexture(GL_TEXTURE_2D, tex_id)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_tex, h_tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data)

            glEnable(GL_TEXTURE_2D)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE)
            has_texture = True
        except Exception as e:
            print(f"[ReCraft 3D] Ошибка загрузки terrain.png: {e}")

    w, d, h = level.width, level.depth, level.height
    grid = np.frombuffer(level.blocks, dtype=np.uint8).reshape((d, h, w))

    # Категоризация блоков
    water_mask = np.isin(grid, list(WATER_BLOCKS))
    lava_mask = np.isin(grid, list(LAVA_BLOCKS))
    liquid_mask = water_mask | lava_mask
    bush_mask = np.isin(grid, list(BUSH_BLOCKS))
    transp_mask = np.isin(grid, list(TRANSPARENT_BLOCKS))
    
    # Твердые кубические блоки (не воздух, не растения, не жидкости)
    solid_mask = (grid != 0) & (~bush_mask) & (~liquid_mask)

    # Каллинг для обычных твердых блоков (грани видны, если сосед — воздух, растение или прозрачный)
    # Вода считается перекрывающей грань блок, как в оригинале
    solid_open = transp_mask | liquid_mask

    def calc_vis(target_mask, neighbor_open_mask):
        top = np.zeros_like(target_mask, dtype=bool)
        top[:-1, :, :] = target_mask[:-1, :, :] & (neighbor_open_mask[1:, :, :])
        top[-1, :, :] = target_mask[-1, :, :]

        bot = np.zeros_like(target_mask, dtype=bool)
        bot[1:, :, :] = target_mask[1:, :, :] & (neighbor_open_mask[:-1, :, :])
        bot[0, :, :] = target_mask[0, :, :]

        north = np.zeros_like(target_mask, dtype=bool)
        north[:, 1:, :] = target_mask[:, 1:, :] & (neighbor_open_mask[:, :-1, :])
        north[:, 0, :] = target_mask[:, 0, :]

        south = np.zeros_like(target_mask, dtype=bool)
        south[:, :-1, :] = target_mask[:, :-1, :] & (neighbor_open_mask[:, 1:, :])
        south[:, -1, :] = target_mask[:, -1, :]

        west = np.zeros_like(target_mask, dtype=bool)
        west[:, :, 1:] = target_mask[:, :, 1:] & (neighbor_open_mask[:, :, :-1])
        west[:, :, 0] = target_mask[:, :, 0]

        east = np.zeros_like(target_mask, dtype=bool)
        east[:, :, :-1] = target_mask[:, :, :-1] & (neighbor_open_mask[:, :, 1:])
        east[:, :, -1] = target_mask[:, :, -1]

        return top, bot, north, south, west, east

    # 1. Видимость обычных блоков
    s_top, s_bot, s_north, s_south, s_west, s_east = calc_vis(solid_mask, transp_mask)

    # 2. Видимость воды (по LiquidTile::shouldRenderFace):
    # Грань воды видна только если сосед НЕ вода (не входит в WATER_BLOCKS) и при этом открыт (воздух/прозрачный)
    water_open_neighbor = transp_mask & (~water_mask)
    w_top, w_bot, w_north, w_south, w_west, w_east = calc_vis(water_mask, water_open_neighbor)

    # 3. Видимость лавы (аналогично воде, не рендерится внутри лавовых озер)
    lava_open_neighbor = transp_mask & (~lava_mask)
    l_top, l_bot, l_north, l_south, l_west, l_east = calc_vis(lava_mask, lava_open_neighbor)

    # Объединяем кубические грани
    top_vis = s_top | w_top | l_top
    bot_vis = s_bot | w_bot | l_bot
    north_vis = s_north | w_north | l_north
    south_vis = s_south | w_south | l_south
    west_vis = s_west | w_west | l_west
    east_vis = s_east | w_east | l_east

    # Двусторонние верхние и нижние грани для воды (LiquidTile::renderFace -> renderBackFace)
    # Позволяет видеть поверхность воды снизу при нахождении под водой
    w_top_back = w_top
    w_bot_back = w_bot

    palette = np.zeros((256, 4), dtype=np.float32)
    uv_indices = np.zeros((256, 3), dtype=np.int32)

    for tid, info in TILES.items():
        r, g, b = hex_to_rgb(info[1])
        # Вода и лава абсолютно непрозрачны (alpha = 1.0), полупрозрачно только стекло (ID 20)
        alpha = 0.65 if tid == 20 else 1.0
        palette[tid] = (r, g, b, alpha)
        uv_indices[tid] = info[2]

    cube_verts, cube_colors, cube_uvs = [], [], []
    bush_verts, bush_colors, bush_uvs = [], [], []

    tile_size = 1.0 / 16.0
    u_base = (np.arange(256) % 16) * tile_size
    v_base = (np.arange(256) // 16) * tile_size

    def add_cube_faces(vis_mask, face_verts, brightness, face_type_idx, uv_order):
        coords = np.argwhere(vis_mask)
        if len(coords) == 0:
            return
        y = coords[:, 0].astype(np.float32)
        z = coords[:, 1].astype(np.float32)
        x = coords[:, 2].astype(np.float32)
        tiles_id = grid[vis_mask]

        if has_texture:
            base_col = np.ones((len(coords), 4), dtype=np.float32)
            base_col[:, :3] *= brightness
            # Полупрозрачность стекла
            glass_ids = (tiles_id == 20)
            base_col[glass_ids, 3] = 0.65
        else:
            base_col = palette[tiles_id].copy()
            base_col[:, :3] *= brightness

        col_quad = np.repeat(base_col, 4, axis=0)

        v = np.zeros((len(coords), 4, 3), dtype=np.float32)
        for i, pt in enumerate(face_verts):
            v[:, i, 0] = x + pt[0]
            v[:, i, 1] = y + pt[1]
            v[:, i, 2] = z + pt[2]

        cube_verts.append(v.reshape(-1, 3))
        cube_colors.append(col_quad)

        if has_texture:
            t_idx = uv_indices[tiles_id, face_type_idx]
            u0 = u_base[t_idx]
            v0 = v_base[t_idx]
            u1 = u0 + tile_size
            v1 = v0 + tile_size

            uv_candidates = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
            t_uv = np.zeros((len(coords), 4, 2), dtype=np.float32)
            for i, order_idx in enumerate(uv_order):
                u_sel, v_sel = uv_candidates[order_idx]
                t_uv[:, i, 0] = u_sel
                t_uv[:, i, 1] = v_sel

            cube_uvs.append(t_uv.reshape(-1, 2))

    # 1. Кубические грани блоков и жидкостей
    add_cube_faces(bot_vis,   [(0, 0, 0), (1, 0, 0), (1, 0, 1), (0, 0, 1)], 0.5, 1, [0, 1, 2, 3])
    add_cube_faces(top_vis,   [(0, 1, 1), (1, 1, 1), (1, 1, 0), (0, 1, 0)], 1.0, 0, [3, 2, 1, 0])
    add_cube_faces(north_vis, [(0, 0, 0), (0, 1, 0), (1, 1, 0), (1, 0, 0)], 0.8, 2, [3, 0, 1, 2])
    add_cube_faces(south_vis, [(1, 0, 1), (1, 1, 1), (0, 1, 1), (0, 0, 1)], 0.8, 2, [2, 1, 0, 3])
    add_cube_faces(west_vis,  [(0, 0, 1), (0, 1, 1), (0, 1, 0), (0, 0, 0)], 0.65, 2, [2, 1, 0, 3])
    add_cube_faces(east_vis,  [(1, 0, 0), (1, 1, 0), (1, 1, 1), (1, 0, 1)], 0.65, 2, [2, 1, 0, 3])

    # 2. Двусторонние обратные грани для воды (LiquidTile::renderBackFace)
    # Нижняя грань изнутри:
    add_cube_faces(w_bot_back, [(0, 0, 1), (1, 0, 1), (1, 0, 0), (0, 0, 0)], 0.5, 1, [3, 2, 1, 0])
    # Верхняя грань изнутри:
    add_cube_faces(w_top_back, [(0, 1, 0), (1, 1, 0), (1, 1, 1), (0, 1, 1)], 1.0, 0, [0, 1, 2, 3])

    # 3. Крестообразные X-модели (Bush, цветы, грибы, паутина)
    b_coords = np.argwhere(bush_mask)
    if len(b_coords) > 0:
        by = b_coords[:, 0].astype(np.float32)
        bz = b_coords[:, 1].astype(np.float32)
        bx = b_coords[:, 2].astype(np.float32)
        b_tiles = grid[bush_mask]
        n_bushes = len(b_coords)

        diag_planes = [
            [(0, 1, 0), (1, 1, 1), (1, 0, 1), (0, 0, 0)],
            [(1, 1, 1), (0, 1, 0), (0, 0, 0), (1, 0, 1)],
            [(1, 1, 0), (0, 1, 1), (0, 0, 1), (1, 0, 0)],
            [(0, 1, 1), (1, 1, 0), (1, 0, 0), (0, 0, 1)],
        ]

        if has_texture:
            b_col_base = np.ones((n_bushes, 4), dtype=np.float32)
        else:
            b_col_base = palette[b_tiles]

        for plane in diag_planes:
            v = np.zeros((n_bushes, 4, 3), dtype=np.float32)
            for i, pt in enumerate(plane):
                v[:, i, 0] = bx + pt[0]
                v[:, i, 1] = by + pt[1]
                v[:, i, 2] = bz + pt[2]

            bush_verts.append(v.reshape(-1, 3))
            bush_colors.append(np.repeat(b_col_base, 4, axis=0))

            if has_texture:
                t_idx = uv_indices[b_tiles, 0]
                u0 = u_base[t_idx]
                v0 = v_base[t_idx]
                u1 = u0 + tile_size
                v1 = v0 + tile_size

                t_uv = np.zeros((n_bushes, 4, 2), dtype=np.float32)
                t_uv[:, 0, 0], t_uv[:, 0, 1] = u0, v0
                t_uv[:, 1, 0], t_uv[:, 1, 1] = u1, v0
                t_uv[:, 2, 0], t_uv[:, 2, 1] = u1, v1
                t_uv[:, 3, 0], t_uv[:, 3, 1] = u0, v1
                bush_uvs.append(t_uv.reshape(-1, 2))

    if cube_verts:
        all_cube_verts = np.concatenate(cube_verts, axis=0).astype(np.float32)
        all_cube_colors = np.concatenate(cube_colors, axis=0).astype(np.float32)
        all_cube_uvs = np.concatenate(cube_uvs, axis=0).astype(np.float32) if has_texture else None
        cube_count = len(all_cube_verts)
    else:
        all_cube_verts = np.zeros((0, 3), dtype=np.float32)
        all_cube_colors = np.zeros((0, 4), dtype=np.float32)
        all_cube_uvs = None
        cube_count = 0

    if bush_verts:
        all_bush_verts = np.concatenate(bush_verts, axis=0).astype(np.float32)
        all_bush_colors = np.concatenate(bush_colors, axis=0).astype(np.float32)
        all_bush_uvs = np.concatenate(bush_uvs, axis=0).astype(np.float32) if has_texture else None
        bush_count = len(all_bush_verts)
    else:
        all_bush_verts = np.zeros((0, 3), dtype=np.float32)
        all_bush_colors = np.zeros((0, 4), dtype=np.float32)
        all_bush_uvs = None
        bush_count = 0

    cam_x, cam_y, cam_z = float(level.spawn_x), float(level.spawn_y + 4), float(level.spawn_z)
    yaw, pitch = 180.0, 0.0

    clock = pygame.time.Clock()
    running = True

    while running:
        dt = clock.tick(60) / 1000.0

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
            elif event.type == pygame.MOUSEMOTION:
                dx, dy = event.rel
                yaw += dx * 0.15
                pitch = max(-89.0, min(89.0, pitch + dy * 0.15))

        keys = pygame.key.get_pressed()
        speed = 35.0 * dt
        if keys[pygame.K_LSHIFT]:
            speed *= 2.5

        rad_y = math.radians(yaw)
        sin_y, cos_y = math.sin(rad_y), math.cos(rad_y)

        if keys[pygame.K_w]:
            cam_x += sin_y * speed
            cam_z -= cos_y * speed
        if keys[pygame.K_s]:
            cam_x -= sin_y * speed
            cam_z += cos_y * speed
        if keys[pygame.K_a]:
            cam_x -= cos_y * speed
            cam_z -= sin_y * speed
        if keys[pygame.K_d]:
            cam_x += cos_y * speed
            cam_z += sin_y * speed
        if keys[pygame.K_SPACE]:
            cam_y += speed
        if keys[pygame.K_LCTRL]:
            cam_y -= speed

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        glRotatef(pitch, 1, 0, 0)
        glRotatef(yaw, 0, 1, 0)
        glTranslatef(-cam_x, -cam_y, -cam_z)

        glEnableClientState(GL_VERTEX_ARRAY)
        glEnableClientState(GL_COLOR_ARRAY)

        # 1. Рендер блоков и жидкостей с отсечением невидимых граней (Culling)
        if cube_count > 0:
            glEnable(GL_CULL_FACE)
            glVertexPointer(3, GL_FLOAT, 0, all_cube_verts)
            glColorPointer(4, GL_FLOAT, 0, all_cube_colors)

            if has_texture and all_cube_uvs is not None:
                glEnable(GL_TEXTURE_2D)
                glEnableClientState(GL_TEXTURE_COORD_ARRAY)
                glTexCoordPointer(2, GL_FLOAT, 0, all_cube_uvs)
            else:
                glDisable(GL_TEXTURE_2D)

            glDrawArrays(GL_QUADS, 0, cube_count)

        # 2. Рендер X-моделей (цветы, трава, кусты, паутина) без отсечения
        if bush_count > 0:
            glDisable(GL_CULL_FACE)
            glVertexPointer(3, GL_FLOAT, 0, all_bush_verts)
            glColorPointer(4, GL_FLOAT, 0, all_bush_colors)

            if has_texture and all_bush_uvs is not None:
                glEnable(GL_TEXTURE_2D)
                glEnableClientState(GL_TEXTURE_COORD_ARRAY)
                glTexCoordPointer(2, GL_FLOAT, 0, all_bush_uvs)
            else:
                glDisable(GL_TEXTURE_2D)

            glDrawArrays(GL_QUADS, 0, bush_count)

        if has_texture:
            glDisableClientState(GL_TEXTURE_COORD_ARRAY)

        glDisableClientState(GL_COLOR_ARRAY)
        glDisableClientState(GL_VERTEX_ARRAY)

        pygame.display.flip()

    pygame.quit()


class WorldEditorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("ReCraft Level Editor")
        self.root.geometry("1240x820")

        self.level = LevelData()
        self.current_y = 32
        self.selected_tile = 1
        self.zoom = 3

        self.terrain_path = "terrain.png" if os.path.isfile("terrain.png") else ""

        self.create_ui()
        self.update_stats_ui()
        self.redraw_canvas()

    def create_ui(self):
        top_bar = tk.Frame(self.root, relief=tk.RAISED, bd=2)
        top_bar.pack(side=tk.TOP, fill=tk.X, padx=4, pady=4)

        tk.Button(top_bar, text="Открыть...", command=self.open_file).pack(side=tk.LEFT, padx=3)
        tk.Button(top_bar, text="Сохранить как...", command=self.save_file_dialog).pack(side=tk.LEFT, padx=3)

        tk.Label(top_bar, text="Формат:").pack(side=tk.LEFT, padx=(8, 2))
        self.proto_var = tk.StringVar(value="Client v4 (Classic Gzip)")
        self.proto_box = ttk.Combobox(
            top_bar,
            textvariable=self.proto_var,
            values=[
                "Client v4 (Classic Gzip)",
                "Client v3 (Classic Gzip)",
                "Client v2 (Classic Gzip)",
                "Server v1 (Raw Binary)",
            ],
            state="readonly",
            width=22,
        )
        self.proto_box.pack(side=tk.LEFT, padx=3)

        self.btn_tex = tk.Button(top_bar, text="Загрузить terrain.png...", command=self.choose_terrain)
        self.btn_tex.pack(side=tk.LEFT, padx=5)

        self.lbl_tex_status = tk.Label(
            top_bar,
            text="[Текстура: OK]" if self.terrain_path else "[Без текстур]",
            fg="#27AE60" if self.terrain_path else "#888888",
            font=("Arial", 8, "bold"),
        )
        self.lbl_tex_status.pack(side=tk.LEFT, padx=3)

        tk.Button(
            top_bar,
            text="3D Обзор (OpenGL GPU)",
            bg="#27AE60",
            fg="white",
            font=("Arial", 9, "bold"),
            command=self.open_3d_viewer,
        ).pack(side=tk.LEFT, padx=10)

        self.lbl_dims = tk.Label(top_bar, text="Размер: 64x64x64", font=("Arial", 9, "bold"))
        self.lbl_dims.pack(side=tk.RIGHT, padx=10)

        main_paned = tk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True)

        sidebar = tk.Frame(main_paned, width=360)
        main_paned.add(sidebar)

        scroll_canvas = tk.Canvas(sidebar, borderwidth=0, width=350)
        sb_scrollbar = ttk.Scrollbar(sidebar, orient="vertical", command=scroll_canvas.yview)
        scroll_content = tk.Frame(scroll_canvas)

        scroll_canvas.create_window((0, 0), window=scroll_content, anchor="nw")
        scroll_canvas.configure(yscrollcommand=sb_scrollbar.set)
        scroll_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        sb_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        scroll_content.bind("<Configure>", lambda e: scroll_canvas.configure(scrollregion=scroll_canvas.bbox("all")))

        replace_group = tk.LabelFrame(
            scroll_content,
            text="Утилита замены блоков (Replace)",
            padx=6,
            pady=6,
            fg="#2C3E50",
            font=("Arial", 9, "bold"),
        )
        replace_group.pack(fill=tk.X, padx=5, pady=4)

        self.tile_options = [f"{tid}: {info[0]}" for tid, info in sorted(TILES.items())]

        tk.Label(replace_group, text="Заменить (From):").grid(row=0, column=0, sticky="w")
        self.rep_from_var = tk.StringVar(value=self.tile_options[1])
        self.rep_from_cb = ttk.Combobox(
            replace_group,
            textvariable=self.rep_from_var,
            values=self.tile_options,
            state="readonly",
            width=22,
        )
        self.rep_from_cb.grid(row=0, column=1, pady=2, sticky="ew")

        tk.Label(replace_group, text="На (To):").grid(row=1, column=0, sticky="w")
        self.rep_to_var = tk.StringVar(value=self.tile_options[4])
        self.rep_to_cb = ttk.Combobox(
            replace_group,
            textvariable=self.rep_to_var,
            values=self.tile_options,
            state="readonly",
            width=22,
        )
        self.rep_to_cb.grid(row=1, column=1, pady=2, sticky="ew")

        self.rep_layer_only = tk.BooleanVar(value=False)
        tk.Checkbutton(
            replace_group, text="Только на текущем слое Y", variable=self.rep_layer_only
        ).grid(row=2, column=0, columnspan=2, sticky="w", pady=2)

        btn_rep = tk.Button(
            replace_group,
            text="Выполнить замену",
            bg="#27AE60",
            fg="white",
            command=self.apply_replace,
        )
        btn_rep.grid(row=3, column=0, columnspan=2, sticky="ew", pady=3)

        brush_group = tk.LabelFrame(
            scroll_content, text="Кисть (ЛКМ - рисовать, ПКМ - заливка)", padx=5, pady=5
        )
        brush_group.pack(fill=tk.X, padx=5, pady=3)

        self.tile_combo_var = tk.StringVar(value=self.tile_options[1])
        self.tile_combo = ttk.Combobox(
            brush_group,
            textvariable=self.tile_combo_var,
            values=self.tile_options,
            state="readonly",
            width=28,
        )
        self.tile_combo.pack(fill=tk.X, pady=3)
        self.tile_combo.bind("<<ComboboxSelected>>", self.on_tile_combo_select)

        preview_f = tk.Frame(brush_group)
        preview_f.pack(fill=tk.X, pady=2)
        tk.Label(preview_f, text="Цвет кисти:").pack(side=tk.LEFT)
        self.color_preview = tk.Label(preview_f, width=4, relief=tk.SUNKEN, bg=TILES[1][1])
        self.color_preview.pack(side=tk.LEFT, padx=6)

        meta_group = tk.LabelFrame(scroll_content, text="Метаданные мира", padx=5, pady=5)
        meta_group.pack(fill=tk.X, padx=5, pady=3)

        tk.Label(meta_group, text="Имя мира:").grid(row=0, column=0, sticky="w")
        self.ent_name = tk.Entry(meta_group)
        self.ent_name.grid(row=0, column=1, pady=2, sticky="ew")

        tk.Label(meta_group, text="Создатель:").grid(row=1, column=0, sticky="w")
        self.ent_creator = tk.Entry(meta_group)
        self.ent_creator.grid(row=1, column=1, pady=2, sticky="ew")

        spawn_group = tk.LabelFrame(scroll_content, text="Spawn", padx=5, pady=5)
        spawn_group.pack(fill=tk.X, padx=5, pady=3)

        tk.Label(spawn_group, text="X:").grid(row=0, column=0)
        self.ent_sp_x = tk.Entry(spawn_group, width=5)
        self.ent_sp_x.grid(row=0, column=1)

        tk.Label(spawn_group, text="Y:").grid(row=0, column=2)
        self.ent_sp_y = tk.Entry(spawn_group, width=5)
        self.ent_sp_y.grid(row=0, column=3)

        tk.Label(spawn_group, text="Z:").grid(row=0, column=4)
        self.ent_sp_z = tk.Entry(spawn_group, width=5)
        self.ent_sp_z.grid(row=0, column=5)

        tk.Label(spawn_group, text="Rot:").grid(row=0, column=6)
        self.ent_sp_rot = tk.Entry(spawn_group, width=5)
        self.ent_sp_rot.grid(row=0, column=7)

        stat_group = tk.LabelFrame(scroll_content, text="Параметры игрока", padx=5, pady=5)
        stat_group.pack(fill=tk.X, padx=5, pady=3)

        tk.Label(stat_group, text="HP:").grid(row=0, column=0, sticky="w")
        self.ent_hp = tk.Entry(stat_group, width=7)
        self.ent_hp.grid(row=0, column=1, pady=2)

        tk.Label(stat_group, text="Воздух:").grid(row=0, column=2, sticky="w")
        self.ent_air = tk.Entry(stat_group, width=7)
        self.ent_air.grid(row=0, column=3, pady=2)

        tk.Label(stat_group, text="Счет:").grid(row=1, column=0, sticky="w")
        self.ent_score = tk.Entry(stat_group, width=7)
        self.ent_score.grid(row=1, column=1, pady=2)

        tk.Label(stat_group, text="Стрелы:").grid(row=1, column=2, sticky="w")
        self.ent_arrows = tk.Entry(stat_group, width=7)
        self.ent_arrows.grid(row=1, column=3, pady=2)

        inv_group = tk.LabelFrame(scroll_content, text="Хотбар (ID / Кол-во)", padx=5, pady=5)
        inv_group.pack(fill=tk.X, padx=5, pady=3)

        self.slot_entries = []
        for i in range(9):
            row_f = tk.Frame(inv_group)
            row_f.pack(fill=tk.X, pady=1)
            tk.Label(row_f, text=f"{i+1}:", width=3, anchor="w").pack(side=tk.LEFT)
            s_id = tk.Entry(row_f, width=6)
            s_id.pack(side=tk.LEFT, padx=2)
            s_cnt = tk.Entry(row_f, width=6)
            s_cnt.pack(side=tk.LEFT, padx=2)
            self.slot_entries.append((s_id, s_cnt))

        btn_flat = tk.Button(
            scroll_content, text="Сгенерировать суперплоский мир", command=self.generate_flat_world
        )
        btn_flat.pack(fill=tk.X, padx=5, pady=6)

        map_container = tk.Frame(main_paned)
        main_paned.add(map_container)

        ctrl_bar = tk.Frame(map_container)
        ctrl_bar.pack(side=tk.TOP, fill=tk.X, padx=5, pady=4)

        tk.Label(ctrl_bar, text="Срез Y:").pack(side=tk.LEFT)
        self.scale_y = tk.Scale(
            ctrl_bar,
            from_=0,
            to=63,
            orient=tk.HORIZONTAL,
            command=self.on_layer_change,
            length=320,
        )
        self.scale_y.set(32)
        self.scale_y.pack(side=tk.LEFT, padx=10)

        self.lbl_layer = tk.Label(ctrl_bar, text="Y = 32")
        self.lbl_layer.pack(side=tk.LEFT)

        tk.Button(ctrl_bar, text="Масштаб +", command=self.zoom_in).pack(side=tk.RIGHT, padx=2)
        tk.Button(ctrl_bar, text="Масштаб -", command=self.zoom_out).pack(side=tk.RIGHT, padx=2)

        canvas_frame = tk.Frame(map_container)
        canvas_frame.pack(fill=tk.BOTH, expand=True)

        self.canvas_scroll_x = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL)
        self.canvas_scroll_y = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL)

        self.canvas = tk.Canvas(
            canvas_frame,
            bg="#0D0D0D",
            xscrollcommand=self.canvas_scroll_x.set,
            yscrollcommand=self.canvas_scroll_y.set,
        )
        self.canvas_scroll_x.config(command=self.canvas.xview)
        self.canvas_scroll_y.config(command=self.canvas.yview)

        self.canvas_scroll_x.pack(side=tk.BOTTOM, fill=tk.X)
        self.canvas_scroll_y.pack(side=tk.RIGHT, fill=tk.Y)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.canvas.bind("<Button-1>", self.on_canvas_click)
        self.canvas.bind("<B1-Motion>", self.on_canvas_click)
        self.canvas.bind("<Button-3>", self.on_canvas_right_click)

    def choose_terrain(self):
        path = filedialog.askopenfilename(
            title="Выберите terrain.png",
            filetypes=[("PNG Image", "*.png"), ("All Files", "*.*")],
        )
        if path:
            self.terrain_path = path
            self.lbl_tex_status.config(text="[Текстура: OK]", fg="#27AE60")
            messagebox.showinfo("Текстуры", f"Текстурный атлас загружен:\n{path}")

    def open_3d_viewer(self):
        t = threading.Thread(
            target=run_opengl_viewer, args=(self.level, self.terrain_path), daemon=True
        )
        t.start()

    def apply_replace(self):
        from_id = int(self.rep_from_var.get().split(":")[0])
        to_id = int(self.rep_to_var.get().split(":")[0])
        layer_only = self.rep_layer_only.get()

        target_y = self.current_y if layer_only else None
        changed = self.level.replace_blocks(from_id, to_id, target_y)
        self.redraw_canvas()
        scope = f"на слое Y={self.current_y}" if layer_only else "во всем мире"
        messagebox.showinfo("Замена выполнена", f"Заменено блоков: {changed} ({scope})")

    def generate_flat_world(self):
        if not messagebox.askyesno("Подтверждение", "Заменить текущий мир на плоский?"):
            return
        w, h, d = self.level.width, self.level.height, self.level.depth
        self.level.blocks = bytearray(w * h * d)
        for y in range(d):
            for z in range(h):
                for x in range(w):
                    if y == 0:
                        self.level.set_block(x, y, z, 7)
                    elif 1 <= y <= 20:
                        self.level.set_block(x, y, z, 3)
                    elif y == 21:
                        self.level.set_block(x, y, z, 2)
        self.level.spawn_x, self.level.spawn_y, self.level.spawn_z = w // 2, 22, h // 2
        self.update_stats_ui()
        self.redraw_canvas()

    def on_tile_combo_select(self, event=None):
        val = self.tile_combo_var.get()
        tid = int(val.split(":")[0])
        self.selected_tile = tid
        color = TILES.get(tid, ("", "#444444"))[1]
        self.color_preview.config(bg=color)

    def on_layer_change(self, val):
        self.current_y = int(val)
        self.lbl_layer.config(text=f"Y = {self.current_y}")
        self.redraw_canvas()

    def zoom_in(self):
        self.zoom = min(16, self.zoom + 1)
        self.redraw_canvas()

    def zoom_out(self):
        self.zoom = max(1, self.zoom - 1)
        self.redraw_canvas()

    def update_stats_ui(self):
        self.ent_name.delete(0, tk.END)
        self.ent_name.insert(0, self.level.name)

        self.ent_creator.delete(0, tk.END)
        self.ent_creator.insert(0, self.level.creator)

        self.ent_sp_x.delete(0, tk.END)
        self.ent_sp_x.insert(0, str(self.level.spawn_x))

        self.ent_sp_y.delete(0, tk.END)
        self.ent_sp_y.insert(0, str(self.level.spawn_y))

        self.ent_sp_z.delete(0, tk.END)
        self.ent_sp_z.insert(0, str(self.level.spawn_z))

        self.ent_sp_rot.delete(0, tk.END)
        self.ent_sp_rot.insert(0, str(self.level.spawn_rot))

        self.ent_hp.delete(0, tk.END)
        self.ent_hp.insert(0, str(self.level.health))

        self.ent_air.delete(0, tk.END)
        self.ent_air.insert(0, str(self.level.air_supply))

        self.ent_score.delete(0, tk.END)
        self.ent_score.insert(0, str(self.level.score))

        self.ent_arrows.delete(0, tk.END)
        self.ent_arrows.insert(0, str(self.level.arrows))

        for i in range(9):
            s_id, s_cnt = self.slot_entries[i]
            s_id.delete(0, tk.END)
            s_id.insert(0, str(self.level.inv_slots[i]))
            s_cnt.delete(0, tk.END)
            s_cnt.insert(0, str(self.level.inv_counts[i]))

        max_y = max(0, self.level.depth - 1)
        self.scale_y.config(to=max_y)
        if self.current_y > max_y:
            self.current_y = max_y
            self.scale_y.set(max_y)

        self.lbl_dims.config(
            text=f"Размер: {self.level.width}x{self.level.depth}x{self.level.height} (WxYxZ)"
        )

    def sync_ui_to_level(self):
        self.level.name = self.ent_name.get()
        self.level.creator = self.ent_creator.get()
        try:
            self.level.spawn_x = int(self.ent_sp_x.get())
            self.level.spawn_y = int(self.ent_sp_y.get())
            self.level.spawn_z = int(self.ent_sp_z.get())
            self.level.spawn_rot = int(self.ent_sp_rot.get())

            self.level.health = int(self.ent_hp.get())
            self.level.air_supply = int(self.ent_air.get())
            self.level.score = int(self.ent_score.get())
            self.level.arrows = int(self.ent_arrows.get())

            for i in range(9):
                s_id, s_cnt = self.slot_entries[i]
                self.level.inv_slots[i] = int(s_id.get())
                self.level.inv_counts[i] = int(s_cnt.get())
        except ValueError:
            messagebox.showwarning("Внимание", "Некоторые поля содержат нечисловые значения.")

    def redraw_canvas(self):
        self.canvas.delete("all")
        y = self.current_y
        w = self.level.width
        h = self.level.height
        zm = self.zoom

        self.canvas.config(scrollregion=(0, 0, w * zm, h * zm))

        for z in range(h):
            for x in range(w):
                tile = self.level.get_block(x, y, z)
                if tile != 0:
                    color = TILES.get(tile, ("", "#444444"))[1]
                    self.canvas.create_rectangle(
                        x * zm,
                        z * zm,
                        (x + 1) * zm,
                        (z + 1) * zm,
                        fill=color,
                        outline="",
                    )

        sp_x = self.level.spawn_x
        sp_z = self.level.spawn_z
        self.canvas.create_oval(
            (sp_x - 1) * zm,
            (sp_z - 1) * zm,
            (sp_x + 1) * zm,
            (sp_z + 1) * zm,
            fill="#FF1111",
            outline="#FFFFFF",
            width=1,
        )

    def on_canvas_click(self, event):
        cx = self.canvas.canvasx(event.x)
        cz = self.canvas.canvasy(event.y)
        x = int(cx // self.zoom)
        z = int(cz // self.zoom)

        if 0 <= x < self.level.width and 0 <= z < self.level.height:
            self.level.set_block(x, self.current_y, z, self.selected_tile)
            color = TILES.get(self.selected_tile, ("", "#444444"))[1]
            self.canvas.create_rectangle(
                x * self.zoom,
                z * self.zoom,
                (x + 1) * self.zoom,
                (z + 1) * self.zoom,
                fill=color,
                outline="",
            )

    def on_canvas_right_click(self, event):
        cx = self.canvas.canvasx(event.x)
        cz = self.canvas.canvasy(event.y)
        start_x = int(cx // self.zoom)
        start_z = int(cz // self.zoom)

        if not (0 <= start_x < self.level.width and 0 <= start_z < self.level.height):
            return

        target_tile = self.level.get_block(start_x, self.current_y, start_z)
        fill_tile = self.selected_tile
        if target_tile == fill_tile:
            return

        queue = deque([(start_x, start_z)])
        visited = set([(start_x, start_z)])

        while queue:
            x, z = queue.popleft()
            self.level.set_block(x, self.current_y, z, fill_tile)

            for nx, nz in ((x + 1, z), (x - 1, z), (x, z + 1), (x, z - 1)):
                if 0 <= nx < self.level.width and 0 <= nz < self.level.height:
                    if (
                        (nx, nz) not in visited
                        and self.level.get_block(nx, self.current_y, nz) == target_tile
                    ):
                        visited.add((nx, nz))
                        queue.append((nx, nz))

        self.redraw_canvas()

    def open_file(self):
        path = filedialog.askopenfilename(
            filetypes=[
                ("ReCraft Worlds", "*.cw *.dat *.bin *.lvl *.*"),
                ("All Files", "*.*"),
            ]
        )
        if not path:
            return

        try:
            with open(path, "rb") as f:
                raw_data = f.read()

            if len(raw_data) < 4:
                raise ValueError("Файл слишком мал.")

            if raw_data[0] == 0x1F and raw_data[1] == 0x8B:
                decompressed = gzip.decompress(raw_data)
                self.load_client_world(decompressed)
            else:
                magic = struct.unpack_from("<I", raw_data, 0)[0]
                if magic == SERVER_MAGIC:
                    self.load_server_world(raw_data)
                else:
                    raise ValueError(f"Неизвестный заголовок файла: {hex(magic)}")

            self.update_stats_ui()
            self.redraw_canvas()
            messagebox.showinfo("Успех", f"Мир загружен:\n{path}")
        except Exception as e:
            messagebox.showerror("Ошибка загрузки", f"Не удалось прочитать мир:\n{str(e)}")

    def load_client_world(self, data: bytes):
        r = BinaryReader(data)
        magic = r.read_i32()
        if magic != CLIENT_MAGIC:
            raise ValueError(f"Неверный client magic number: {magic}")

        version = r.read_u8()
        self.level.name = r.read_utf()
        self.level.creator = r.read_utf()
        self.level.creation_time = r.read_i64()

        self.level.width = r.read_i16()
        self.level.height = r.read_i16()  # Z
        self.level.depth = r.read_i16()   # Y

        blk_len = self.level.width * self.level.height * self.level.depth
        self.level.blocks = r.read_bytes(blk_len)
        self.level.entities.clear()

        if version == 2:
            self.proto_var.set("Client v2 (Classic Gzip)")
            self.level.spawn_x = r.read_i16()
            self.level.spawn_y = r.read_i16()
            self.level.spawn_z = r.read_i16()
            self.level.spawn_rot = r.read_i16()
            count = r.read_i32()
            for _ in range(count):
                etype = r.read_i32()
                if etype == 1:
                    self.level.entities.append(
                        {
                            "type": "zombie",
                            "x": r.read_float(),
                            "y": r.read_float(),
                            "z": r.read_float(),
                            "rx": r.read_float(),
                            "ry": r.read_float(),
                        }
                    )
                else:
                    r.offset += 20
        elif version >= 3:
            self.proto_var.set(
                "Client v4 (Classic Gzip)" if version >= 4 else "Client v3 (Classic Gzip)"
            )
            self.level.spawn_x = r.read_i16()
            self.level.spawn_y = r.read_i16()
            self.level.spawn_z = r.read_i16()
            self.level.spawn_rot = r.read_i16()

            self.level.health = r.read_i16()
            self.level.air_supply = r.read_i16()
            self.level.score = r.read_i16()
            self.level.arrows = r.read_i16()

            for i in range(9):
                self.level.inv_slots[i] = r.read_i16()
                self.level.inv_counts[i] = r.read_i16()

            count = r.read_i32()
            for _ in range(count):
                etype = r.read_i32()
                if etype == 100:
                    it = r.read_i8()
                    if it == 1:
                        res = r.read_i8()
                        self.level.entities.append(
                            {
                                "type": "item",
                                "id": res,
                                "x": r.read_float(),
                                "y": r.read_float(),
                                "z": r.read_float(),
                            }
                        )
                    elif it == 2:
                        at = r.read_i8()
                        if at == 0:
                            hit = r.read_bool()
                            gr = r.read_float()
                            self.level.entities.append(
                                {
                                    "type": "arrow",
                                    "hit": hit,
                                    "grav": gr,
                                    "x": r.read_float(),
                                    "y": r.read_float(),
                                    "z": r.read_float(),
                                    "rx": r.read_float(),
                                    "ry": r.read_float(),
                                }
                            )
                else:
                    self.level.entities.append(
                        {
                            "type": "mob",
                            "mob_id": etype,
                            "x": r.read_float(),
                            "y": r.read_float(),
                            "z": r.read_float(),
                        }
                    )

    def load_server_world(self, data: bytes):
        self.proto_var.set("Server v1 (Raw Binary)")
        r = BinaryReader(data)

        magic = r.read_u32_le()
        ver = r.read_u32_le()
        if magic != SERVER_MAGIC or ver != 1:
            raise ValueError("Некорректный формат серверного уровня.")

        self.level.width = r.read_i32_le()
        self.level.height = r.read_i32_le()
        self.level.depth = r.read_i32_le()

        self.level.spawn_x = r.read_i32_le()
        self.level.spawn_y = r.read_i32_le()
        self.level.spawn_z = r.read_i32_le()
        self.level.spawn_rot = r.read_i32_le()

        blk_len = self.level.width * self.level.height * self.level.depth
        self.level.blocks = r.read_bytes(blk_len)

    def save_file_dialog(self):
        self.sync_ui_to_level()
        path = filedialog.asksaveasfilename(
            defaultextension=".dat",
            filetypes=[
                ("ReCraft Save (*.dat)", "*.dat"),
                ("Server Level (*.bin)", "*.bin"),
                ("All Files", "*.*"),
            ],
        )
        if not path:
            return

        proto = self.proto_var.get()
        try:
            if "Server v1" in proto:
                data = self.serialize_server()
                with open(path, "wb") as f:
                    f.write(data)
            else:
                ver = 4
                if "Client v3" in proto:
                    ver = 3
                elif "Client v2" in proto:
                    ver = 2

                raw_bytes = self.serialize_client(ver)
                compressed = gzip.compress(raw_bytes)
                with open(path, "wb") as f:
                    f.write(compressed)

            messagebox.showinfo("Успех", f"Мир сохранен в формате:\n{proto}")
        except Exception as e:
            messagebox.showerror("Ошибка сохранения", f"Не удалось записать файл:\n{str(e)}")

    def serialize_client(self, ver: int) -> bytearray:
        w = BinaryWriter()
        w.write_i32(CLIENT_MAGIC)
        w.write_u8(ver)

        w.write_utf(self.level.name)
        w.write_utf(self.level.creator)
        w.write_i64(self.level.creation_time)

        w.write_i16(self.level.width)
        w.write_i16(self.level.height)
        w.write_i16(self.level.depth)
        w.write_bytes(self.level.blocks)

        if ver == 2:
            w.write_i16(self.level.spawn_x)
            w.write_i16(self.level.spawn_y)
            w.write_i16(self.level.spawn_z)
            w.write_i16(self.level.spawn_rot)
            zombies = [
                e
                for e in self.level.entities
                if e.get("type") == "zombie" or e.get("mob_id") == 1
            ]
            w.write_i32(len(zombies))
            for z in zombies:
                w.write_i32(1)
                w.write_float(z.get("x", 0.0))
                w.write_float(z.get("y", 0.0))
                w.write_float(z.get("z", 0.0))
                w.write_float(z.get("rx", 0.0))
                w.write_float(z.get("ry", 0.0))
        elif ver >= 3:
            w.write_i16(self.level.spawn_x)
            w.write_i16(self.level.spawn_y)
            w.write_i16(self.level.spawn_z)
            w.write_i16(self.level.spawn_rot)

            w.write_i16(self.level.health)
            w.write_i16(self.level.air_supply)
            w.write_i16(self.level.score)
            w.write_i16(self.level.arrows)

            for i in range(9):
                w.write_i16(self.level.inv_slots[i])
                w.write_i16(self.level.inv_counts[i])

            w.write_i32(len(self.level.entities))
            for e in self.level.entities:
                if e.get("type") == "item":
                    w.write_i32(100)
                    w.write_u8(1)
                    w.write_u8(e.get("id", 1))
                    w.write_float(e.get("x", 0.0))
                    w.write_float(e.get("y", 0.0))
                    w.write_float(e.get("z", 0.0))
                elif e.get("type") == "arrow":
                    w.write_i32(100)
                    w.write_u8(2)
                    w.write_u8(0)
                    w.write_bool(e.get("hit", False))
                    w.write_float(e.get("grav", 0.04))
                    w.write_float(e.get("x", 0.0))
                    w.write_float(e.get("y", 0.0))
                    w.write_float(e.get("z", 0.0))
                    w.write_float(e.get("rx", 0.0))
                    w.write_float(e.get("ry", 0.0))
                else:
                    w.write_i32(e.get("mob_id", 1))
                    w.write_float(e.get("x", 0.0))
                    w.write_float(e.get("y", 0.0))
                    w.write_float(e.get("z", 0.0))

        return w.buf

    def serialize_server(self) -> bytes:
        buf = bytearray()
        buf.extend(struct.pack("<I", SERVER_MAGIC))
        buf.extend(struct.pack("<I", 1))

        buf.extend(struct.pack("<i", int(self.level.width)))
        buf.extend(struct.pack("<i", int(self.level.height)))
        buf.extend(struct.pack("<i", int(self.level.depth)))

        buf.extend(struct.pack("<i", int(self.level.spawn_x)))
        buf.extend(struct.pack("<i", int(self.level.spawn_y)))
        buf.extend(struct.pack("<i", int(self.level.spawn_z)))
        buf.extend(struct.pack("<i", int(self.level.spawn_rot)))

        buf.extend(self.level.blocks)
        return bytes(buf)


if __name__ == "__main__":
    tk_root = tk.Tk()
    app = WorldEditorApp(tk_root)
    tk_root.mainloop()