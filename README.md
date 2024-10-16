# FolderToM3UGenerator
    Had a quick need for this so made it.
    Generates an M3U playlist file with a name based on the parent directory.
    Super simple stand-alone win executable to generate m3u playlists from a folder of music files.
    Just drop it in the folder with the music files and run it.
    Meta: Yes it could just be a powershell or bash script, but I wanted to make a standalone executable for portability, simplicity & bit of variety.
    
## Options
	Options are set within the filename of the executable:
    Parses the executable filename for flags (--R+, --A+, --D+), maybe a bit unconventional but it made sense to me for maximum portability/simplicity - set it and forget it.
    Scans the directory (recursively if --R+ flag is present) for audio/video files: ".mp3", ".wav", ".ogg", ".flac", ".m4a", ".aac", ".mp4", ".avi", ".mkv", ".mov", ".wmv" ... fork and tweak to suit your needs if you like, if anyone is likely to want to overcomplicate what could just be a shell script :)
    Sorts files alphabetically (--A+ flag) or by creation date (--D+ flag), if both are set it prioritises Alphabetical and disregards Date.
    FolderToM3UGenerator--R+.exe

## Behaviour
    Keeps contents of subfolders grouped and sorted together, ordered within the overall hierarchy whenever it found them.
    Uses the C++17 filesystem library for file operations
    Super basic input-filename sanitisation sanity check
    Generates error file where any encountered, and uses _getch() to pause for user input if error encountered to offer opportunity to become aware.

## Example
    FolderToM3UGenerator--R+.exe [The --R+ flag tells it to scan recursively]
    FolderToM3UGenerator--A+.exe [The --A+ flag tells it to sort alphabetically]
    FolderToM3UGenerator--D+.exe [The --D+ flag tells it to sort by date]
    FolderToM3UGenerator--R+--A+.exe [The --R+ flag tells it to scan recursively, and the --A+ flag tells it to sort alphabetically]

## For Giggles
### Powershell
    Get-ChildItem -File | Where-Object {$_.Extension -match '\.(mp3|wav|ogg|flac|m4a|aac|mp4|avi|mkv|mov|wmv)$'} | ForEach-Object {$_.Name} | Out-File -FilePath "output.txt"
### Bash
    find . -maxdepth 1 -type f \( -name "*.mp3" -o -name "*.wav" -o -name "*.ogg" -o -name "*.flac" -o -name "*.m4a" -o -name "*.aac" -o -name "*.mp4" -o -name "*.avi" -o -name "*.mkv" -o -name "*.mov" -o -name "*.wmv" \) -printf "%f\n" > output.txt