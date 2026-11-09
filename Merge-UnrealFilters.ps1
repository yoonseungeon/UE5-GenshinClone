[CmdletBinding()]
param([string]$ProjectRoot = '')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) { $ProjectRoot = $PSScriptRoot }

# Only VS display filters are changed; source paths and build settings are untouched.
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path.TrimEnd('\')
$projects = @(Get-ChildItem -LiteralPath $root -Filter '*.uproject' -File)
if ($projects.Count -ne 1) { throw 'Specify a folder containing exactly one .uproject file.' }
$filterPath = Join-Path $root ("Intermediate\ProjectFiles\{0}.vcxproj.filters" -f $projects[0].BaseName)
if (-not (Test-Path -LiteralPath $filterPath -PathType Leaf)) {
    throw 'VS project filters were not found. Generate Visual Studio project files first.'
}

$modules = @(Get-ChildItem -LiteralPath (Join-Path $root 'Source') -Filter '*.Build.cs' -File -Recurse | ForEach-Object {
    [pscustomobject]@{
        Name = $_.Name.Substring(0, $_.Name.Length - '.Build.cs'.Length)
        Directory = $_.Directory.FullName.TrimEnd('\')
        Filter = $_.Directory.FullName.Substring($root.Length + 1)
    }
} | Sort-Object { $_.Directory.Length } -Descending)

$document = New-Object System.Xml.XmlDocument
$document.PreserveWhitespace = $false
$document.Load($filterPath)
$namespace = $document.DocumentElement.NamespaceURI
$manager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
$manager.AddNamespace('m', $namespace)
$oldIds = @{}
foreach ($definition in $document.SelectNodes('/m:Project/m:ItemGroup/m:Filter[@Include]', $manager)) {
    $id = $definition.SelectSingleNode('m:UniqueIdentifier', $manager)
    if ($null -ne $id) { $oldIds[$definition.GetAttribute('Include')] = $id.InnerText }
}

$needed = @{}
$filterDirectory = Split-Path -Parent $filterPath
$items = @($document.SelectNodes('/m:Project/m:ItemGroup/*[@Include and local-name() != "Filter"]', $manager))
foreach ($item in $items) {
    $include = $item.GetAttribute('Include')
    $display = $item.SelectSingleNode('m:Filter', $manager)
    $destination = if ($null -ne $display) { $display.InnerText } else { '' }
    # Keep MSBuild expressions and wildcard items as they were.
    if ($include -notmatch '[$*?]') {
        $absolute = [System.IO.Path]::GetFullPath((Join-Path $filterDirectory $include))
        if ($absolute.StartsWith($root + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = $absolute.Substring($root.Length + 1)
            if ([System.IO.Path]::GetDirectoryName($relative) -eq '') {
                $destination = 'Default'
            } elseif ($relative.StartsWith('Config\', [System.StringComparison]::OrdinalIgnoreCase)) {
                $destination = 'Default\' + [System.IO.Path]::GetDirectoryName($relative)
            } elseif ($relative -match '^Source\\.*\.(Build|Target)\.cs$') {
                $destination = 'Default'
            } else {
                foreach ($module in $modules) {
                    if (-not $absolute.StartsWith($module.Directory + '\', [System.StringComparison]::OrdinalIgnoreCase)) { continue }
                    $inside = $absolute.Substring($module.Directory.Length + 1)
                    # Strip only the Public/Private directory immediately under the module.
                    $inside = $inside -replace '^(Public|Private)\\', ''
                    $fileName = [System.IO.Path]::GetFileName($inside)
                    $parent = [System.IO.Path]::GetDirectoryName($inside)
                    if ($parent -eq '' -and ($fileName -eq ($module.Name + '.h') -or $fileName -eq ($module.Name + '.cpp'))) {
                        $destination = 'Default'
                    } else {
                        $destination = $module.Filter
                        if ($parent -ne '') { $destination += '\' + $parent }
                    }
                    break
                }
            }
        }
    }
    if ($destination -ne '') {
        if ($null -eq $display) {
            $display = $document.CreateElement('Filter', $namespace)
            [void]$item.AppendChild($display)
        }
        $display.InnerText = $destination
        $ancestor = $destination
        while ($ancestor -ne '') {
            $needed[$ancestor] = $true
            $separator = $ancestor.LastIndexOf('\')
            $ancestor = if ($separator -lt 0) { '' } else { $ancestor.Substring(0, $separator) }
        }
    }
}

foreach ($definition in @($document.SelectNodes('/m:Project/m:ItemGroup/m:Filter[@Include]', $manager))) {
    [void]$definition.ParentNode.RemoveChild($definition)
}
foreach ($group in @($document.SelectNodes('/m:Project/m:ItemGroup', $manager))) {
    if (-not $group.HasChildNodes) { [void]$group.ParentNode.RemoveChild($group) }
}
$definitions = $document.CreateElement('ItemGroup', $namespace)
$md5 = [System.Security.Cryptography.MD5]::Create()
try {
    foreach ($name in @($needed.Keys | Sort-Object)) {
        $definition = $document.CreateElement('Filter', $namespace)
        $definition.SetAttribute('Include', $name)
        $identifier = $document.CreateElement('UniqueIdentifier', $namespace)
        if ($oldIds.ContainsKey($name)) {
            $identifier.InnerText = $oldIds[$name]
        } else {
            # Stable display identifiers make repeated runs produce the same output.
            $bytes = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($name.ToLowerInvariant()))
            $guid = New-Object System.Guid -ArgumentList (,$bytes)
            $identifier.InnerText = $guid.ToString('B').ToUpperInvariant()
        }
        [void]$definition.AppendChild($identifier)
        [void]$definitions.AppendChild($definition)
    }
} finally { $md5.Dispose() }
[void]$document.DocumentElement.PrependChild($definitions)
$settings = New-Object System.Xml.XmlWriterSettings
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$settings.Indent = $true
$settings.NewLineChars = [Environment]::NewLine
$buffer = New-Object System.IO.MemoryStream
$writer = [System.Xml.XmlWriter]::Create($buffer, $settings)
try {
    $document.Save($writer)
    $writer.Flush()
    $result = $buffer.ToArray()
} finally {
    $writer.Dispose()
    $buffer.Dispose()
}
$existing = [System.IO.File]::ReadAllBytes($filterPath)
if ([Convert]::ToBase64String($existing) -ne [Convert]::ToBase64String($result)) {
    [System.IO.File]::WriteAllBytes($filterPath, $result)
    Write-Host "Updated: $filterPath"
} else {
    Write-Host "Already up to date: $filterPath"
}
Write-Host 'Reload the project in Visual Studio if prompted. Run again after Generate.'
