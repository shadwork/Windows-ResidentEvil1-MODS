# Get the current date and time formatted for the filename
$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$archiveName = "Release-$timestamp.zip"

# Create a temporary staging directory to hold only the specific files
$tempStagingDir = Join-Path -Path $env:TEMP -ChildPath "ReleaseArchiveStaging_$timestamp"
New-Item -ItemType Directory -Path $tempStagingDir | Out-Null

# Iterate through one-level child folders in the current directory
$subfolders = Get-ChildItem -Directory
$matchCount = 0

foreach ($folder in $subfolders) {
    $folderPath = $folder.FullName
    
    # Define the exact paths to check
    $dllPath         = Join-Path -Path $folderPath -ChildPath "Release\*.dll"
    $manifestPath    = Join-Path -Path $folderPath -ChildPath "manifest.txt"
    $descriptionPath = Join-Path -Path $folderPath -ChildPath "description.txt"
    
    # Check if the required files exist
    $hasDll         = Test-Path -Path $dllPath
    $hasManifest    = Test-Path -Path $manifestPath
    $hasDescription = Test-Path -Path $descriptionPath
    
    # If all three conditions succeed, process the folder
    if ($hasDll -and $hasManifest -and $hasDescription) {
        Write-Host "Match found: $($folder.Name) - Copying files and updating manifest..." -ForegroundColor Green
        $matchCount++
        
        # 1. Create ONLY the parent folder in the temp directory
        $targetFolder = Join-Path -Path $tempStagingDir -ChildPath $folder.Name
        New-Item -ItemType Directory -Path $targetFolder | Out-Null
        
        # 2. Copy the files directly to the target folder (flattening the structure)
        Copy-Item -Path $manifestPath -Destination $targetFolder
        Copy-Item -Path $descriptionPath -Destination $targetFolder
        Copy-Item -Path $dllPath -Destination $targetFolder

        # 3. Copy any .exe files found in the mod folder root (e.g., mod_config.exe)
        $exeFiles = Get-ChildItem -Path $folderPath -Filter "*.exe" -File
        foreach ($exe in $exeFiles) {
            Write-Host "  Including executable: $($exe.Name)" -ForegroundColor Yellow
            Copy-Item -Path $exe.FullName -Destination $targetFolder
        }
        
        # 4. Get the actual name of the found .dll file
        # Using Select-Object -First 1 in case there is somehow more than one .dll in the Release folder
        $foundDll = Get-ChildItem -Path (Join-Path -Path $folderPath -ChildPath "Release") -Filter "*.dll" | Select-Object -First 1
        $dllName = $foundDll.Name
        
        # 5. Modify the staged manifest.txt to update the Module parameter
        $stagedManifest = Join-Path -Path $targetFolder -ChildPath "manifest.txt"
        $manifestContent = Get-Content -Path $stagedManifest
        
        # Regex explanation: Looks for "Module =", ignoring case/spacing, and replaces whatever comes after it with the $dllName
        $manifestContent = $manifestContent -replace '(?i)^(.*Module\s*=\s*).*', ('$1' + $dllName)
        
        # Save the updated text back to the staged manifest file
        Set-Content -Path $stagedManifest -Value $manifestContent
    }
}

# If we found any folders that meet the criteria, pack the staged files
if ($matchCount -gt 0) {
    Write-Host "Packing specific files into $archiveName..." -ForegroundColor Cyan
    Compress-Archive -Path "$tempStagingDir\*" -DestinationPath $archiveName -Force
    Write-Host "Archive created successfully!" -ForegroundColor Green
} else {
    Write-Host "No folders met the required conditions. No archive created." -ForegroundColor Yellow
}

# Clean up the temporary staging directory
Remove-Item -Path $tempStagingDir -Recurse -Force