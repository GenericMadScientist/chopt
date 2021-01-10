import sqlite3
import subprocess

program = "./build/Debug/chopt.exe"
song_dir = "integration_tests/songs"

conn = sqlite3.connect("integration_tests/tests.db")
c = conn.cursor()

c.execute(
    ("select ID, File, Difficulty, Path, BaseScore, TotalScore, AvgMultiplier "
     "from Songs")
)
songs = list(c.fetchall())

outputs = []
for song in songs:
    song_id, _, _, path, base_score, total_score, avg_mult = song
    output_lines = ["Optimising, please wait..."]
    output_lines.append(f"Path: {path}")
    output_lines.append(f"No SP score: {base_score}")
    output_lines.append(f"Total score: {total_score}")
    output_lines.append(f"Average multiplier: {avg_mult}x")
    c.execute(
        ("select Description from Activations where SongID = ? "
         "order by ActivationNumber"),
        (song_id,),
    )
    for description, activation in zip(c.fetchall(), path.split("-")):
        output_lines.append(f"{activation}: {description[0]}")
    output_lines.append("")
    outputs.append("\r\n".join(output_lines).encode("utf-8"))

conn.close()

for song, output in zip(songs, outputs):
    _, file, difficulty, _, _, _, _ = song
    result = subprocess.run(
        [program, "-f", f"{song_dir}/{file}", "-d", difficulty],
        capture_output=True
    )
    result.check_returncode()
    if result.stdout != output:
        print(f"Expected output: {output}")
        print(f"Actual output: {result.stdout}")
        raise RuntimeError(f"Song {file} has incorrect output")
