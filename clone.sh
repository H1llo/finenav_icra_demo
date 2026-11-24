#!/usr/bin/env bash
set -euo pipefail

if (( $# > 1 )); then
  echo "Usage: $0 [target_src_directory]" >&2
  exit 1
fi

command -v git >/dev/null 2>&1 || { echo "Please install git first." >&2; exit 1; }
command -v colcon >/dev/null 2>&1 || { echo "Please install colcon and source the ROS 2 environment first." >&2; exit 1; }

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
target_dir="${1:-$(dirname -- "$script_dir")}"
mkdir -p -- "$target_dir"
target_dir="$(cd -- "$target_dir" && pwd)"
workspace_dir="$(dirname -- "$target_dir")"

repositories=(
  FineNav-Engine
  Ndt_maps
  finenav_map
  finenav_tomography_planner
  tomography_ndt_view
)

for repository in "${repositories[@]}"; do
  destination="$target_dir/$repository"
  if [[ -e "$destination" || -L "$destination" ]]; then
    echo "Skip existing directory: $destination"
    continue
  fi
  git clone -- "https://github.com/FineNav/$repository.git" "$destination"
done

cd -- "$workspace_dir"
echo "Building workspace: $workspace_dir"
colcon build --symlink-install
