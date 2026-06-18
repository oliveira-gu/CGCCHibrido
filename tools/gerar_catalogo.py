from pathlib import Path

TOTAL_COLUNAS = 16
TOTAL_LINHAS = 13

ESPACO_X = 2.0
ESPACO_Z = 2.0

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
OUTPUT_FILE = PROJECT_ROOT / "assets" / "catalogo_basico.txt"

OBJ_PATH = "../assets/Modelos3D/Grass_block.obj"

def main():
    OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)


with OUTPUT_FILE.open("w", encoding="utf-8") as f:
    f.write("camera 15.0 35.0 70.0\n")
    f.write("luz 20.0 40.0 20.0 1.0 1.0 1.0\n\n")

    f.write("# =========================================================\n")
    f.write("# CATALOGO COMPLETO DO TEXTURE ATLAS All_blocks.png\n")
    f.write("# TOTAL_COLUNAS = 16\n")
    f.write("# TOTAL_LINHAS = 55\n")
    f.write("# X = coluna do atlas\n")
    f.write("# Z = linha do atlas\n")
    f.write("# =========================================================\n\n")

    for row in range(TOTAL_LINHAS):
        f.write(f"# ==================== LINHA {row} ====================\n")

        for col in range(TOTAL_COLUNAS):
            pos_x = col * ESPACO_X
            pos_y = 0.0
            pos_z = -row * ESPACO_Z

            f.write(
                f"objeto {OBJ_PATH} "
                f"{col} {row} "
                f"{pos_x:.1f} {pos_y:.1f} {pos_z:.1f} "
                f"0.0 0.0 0.0 1.0 0\n"
            )

        f.write("\n")

total_blocos = TOTAL_COLUNAS * TOTAL_LINHAS

print(f"catalogo.txt gerado com sucesso em:")
print(OUTPUT_FILE)
print(f"Total de blocos: {total_blocos}")


if __name__ == "__main__":
    main()
