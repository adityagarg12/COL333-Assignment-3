#!/usr/bin/env python3
"""
Metro Line Visualizer
Reads .city and .metromap files and creates an interactive visualization
"""

import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.widgets import CheckButtons
import numpy as np
import sys
import os

class MetroVisualizer:
    def __init__(self, city_file, metromap_file):
        self.width = 0
        self.height = 0
        self.max_turns = 0
        self.num_lines = 0
        self.metro_endpoints = []
        self.metro_paths = []
        self.traced_paths = []
        # Updated colors for better visibility and contrast
        self.colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#96CEB4', '#FFEAA7', '#DDA0DD', 
                       '#98D8C8', '#F7DC6F', '#BB8FCE', '#85C1E9', '#F8C471', '#82E0AA']

        self.parse_city_file(city_file)
        self.parse_metromap_file(metromap_file)
        self.trace_all_paths()

    def parse_city_file(self, filename):
        """Parse the .city file to extract city dimensions and metro endpoints"""
        try:
            with open(filename, 'r') as f:
                lines = f.read().strip().split('\n')

            scenario = int(lines[0])
            city_info = list(map(int, lines[1].split()))
            
            if scenario == 1:
                # Scenario 1: width, height, num_lines, max_turns
                self.width, self.height, self.num_lines, self.max_turns = city_info
            else:
                # Scenario 2: width, height, num_lines, max_turns, num_popular_points
                self.width, self.height, self.num_lines, self.max_turns, self.num_popular_points = city_info

            # Parse metro line endpoints
            for i in range(2, 2 + self.num_lines):
                coords = list(map(int, lines[i].split()))
                start_pos = (coords[0], coords[1])
                end_pos = (coords[2], coords[3])
                self.metro_endpoints.append((start_pos, end_pos))

            print(f"City: {self.width}x{self.height}")
            print(f"Max turns: {self.max_turns}")
            print(f"Number of metro lines: {self.num_lines}")

        except Exception as e:
            print(f"Error reading city file: {e}")
            sys.exit(1)

    def parse_metromap_file(self, filename):
        """Parse the .metromap file to extract metro line directions"""
        try:
            with open(filename, 'r') as f:
                lines = f.read().strip().split('\n')

            for line in lines:
                directions = line.split()[:-1]  # Remove the '0' at the end
                self.metro_paths.append(directions)

        except Exception as e:
            print(f"Error reading metromap file: {e}")
            sys.exit(1)

    def trace_metro_path(self, start_pos, directions):
        """Trace a metro path given starting position and directions"""
        path = [start_pos]
        x, y = start_pos

        # Grid system where Y increases downward (matrix indexing)
        direction_map = {
            'U': (0, -1),  # Up decreases Y
            'D': (0, 1),   # Down increases Y
            'L': (-1, 0),  # Left
            'R': (1, 0)    # Right
        }

        for direction in directions:
            if direction in direction_map:
                dx, dy = direction_map[direction]
                x += dx
                y += dy
                path.append((x, y))

        return path

    def count_turns(self, directions):
        """Count the number of turns in a direction sequence"""
        turns = 0
        for i in range(1, len(directions)):
            if directions[i] != directions[i-1]:
                turns += 1
        return turns

    def trace_all_paths(self):
        """Trace all metro line paths and validate them"""
        self.traced_paths = []

        for i, (start_end, directions) in enumerate(zip(self.metro_endpoints, self.metro_paths)):
            start_pos, end_pos = start_end
            path = self.trace_metro_path(start_pos, directions)
            self.traced_paths.append(path)

            turns = self.count_turns(directions)
            matches = path[-1] == end_pos

            print(f"\nMetro Line {i+1}:")
            print(f"  Start: {start_pos} → End: {end_pos}")
            print(f"  Actual end: {path[-1]} ({'✓' if matches else '✗'})")
            print(f"  Turns: {turns}/{self.max_turns} ({'✓' if turns <= self.max_turns else '✗'})")
            print(f"  Direction sequence: {' → '.join(directions)}")

    def create_visualization(self):
        """Create the interactive visualization"""
        fig, ax = plt.subplots(1, 1, figsize=(12, 8))
        plt.subplots_adjust(bottom=0.1, top=0.9, left=0.1, right=0.9)

        # Set up the grid
        ax.set_xlim(-0.5, self.width - 0.5)
        ax.set_ylim(self.height - 0.5, -0.5)  # Invert Y-axis for grid coordinates
        ax.set_aspect('equal')

        # Draw grid
        for i in range(self.width):
            ax.axvline(i, color='lightgray', linewidth=0.5, alpha=0.7)
        for i in range(self.height):
            ax.axhline(i, color='lightgray', linewidth=0.5, alpha=0.7)

        # Set labels
        ax.set_xticks(range(self.width))
        ax.set_yticks(range(self.height))
        ax.set_xlabel('X Coordinate', fontsize=12)
        ax.set_ylabel('Y Coordinate', fontsize=12)
        ax.set_title(f'Metro Line Visualizer - {self.width}×{self.height} City Grid | '
                    f'Max Turns: {self.max_turns} | Lines: {self.num_lines}', 
                    fontsize=14, fontweight='bold')

        # Store line objects for toggling
        self.line_objects = []
        self.start_objects = []
        self.end_objects = []
        self.station_objects = []

        # Draw metro lines with better visual separation
        for i, path in enumerate(self.traced_paths):
            color = self.colors[i % len(self.colors)]

            # Draw the path with slightly thicker lines for better visibility
            xs = [point[0] for point in path]
            ys = [point[1] for point in path]
            line, = ax.plot(xs, ys, color=color, linewidth=4, alpha=0.9, 
                           label=f'Metro Line {i+1}', marker='o', markersize=5)
            self.line_objects.append(line)

            # Draw start point with distinctive styling
            start_marker = ax.scatter(path[0][0], path[0][1], s=200, c=color, 
                                    marker='o', edgecolors='black', linewidths=3,
                                    label=f'Start' if i == 0 else "", zorder=5)
            self.start_objects.append(start_marker)

            # Draw end point with distinctive styling
            end_marker = ax.scatter(path[-1][0], path[-1][1], s=200, c=color, 
                                  marker='s', edgecolors='black', linewidths=3,
                                  label=f'End' if i == 0 else "", zorder=5)
            self.end_objects.append(end_marker)

            # Store station objects for each line
            stations = ax.scatter(xs[1:-1], ys[1:-1], s=60, c=color, 
                                alpha=0.8, marker='o', edgecolors='white', linewidths=1, zorder=3)
            self.station_objects.append(stations)

        # Add compact legend at the bottom
        legend_elements = []
        for i in range(len(self.traced_paths)):
            color = self.colors[i % len(self.colors)]
            legend_elements.append(plt.Line2D([0], [0], color=color, lw=4, label=f'Line {i+1}'))
        
        # Add start/end markers to legend
        legend_elements.append(plt.Line2D([0], [0], marker='o', color='w', markerfacecolor='gray', 
                                        markersize=10, markeredgecolor='black', markeredgewidth=2, 
                                        label='Start', linestyle='None'))
        legend_elements.append(plt.Line2D([0], [0], marker='s', color='w', markerfacecolor='gray', 
                                        markersize=10, markeredgecolor='black', markeredgewidth=2, 
                                        label='End', linestyle='None'))
        
        ax.legend(handles=legend_elements, loc='upper center', bbox_to_anchor=(0.5, -0.05), 
                 ncol=min(6, len(legend_elements)), fontsize=10)

        # Add click handler for station information
        fig.canvas.mpl_connect('button_press_event', self.on_click)

        plt.tight_layout()
        plt.show()

    def on_click(self, event):
        """Handle click events on the plot"""
        if event.inaxes and event.xdata is not None and event.ydata is not None:
            x, y = int(round(event.xdata)), int(round(event.ydata))

            # Check if click is on a metro station
            station_info = []
            for i, path in enumerate(self.traced_paths):
                if (x, y) in path:
                    station_idx = path.index((x, y))
                    distance = station_idx
                    station_info.append(f"Line {i+1}: Station {station_idx} (distance {distance})")

            if station_info:
                info = f"Station at ({x}, {y}):\n" + "\n".join(station_info)
                print(info)
                # Update title with station info
                ax = event.inaxes
                ax.set_title(f'Metro Line Visualizer - {self.width}×{self.height} City Grid | '
                           f'Max Turns: {self.max_turns} | Lines: {self.num_lines} | '
                           f'Station: ({x}, {y})', 
                           fontsize=14, fontweight='bold')
                plt.draw()


def main():
    """Main function"""
    if len(sys.argv) != 3:
        print("Usage: python metro_visualizer.py <city_file> <metromap_file>")
        print("Example: python metro_visualizer.py case_000.city case_000.metromap")
        sys.exit(1)

    city_file = sys.argv[1]
    metromap_file = sys.argv[2]

    # Check if files exist
    if not os.path.exists(city_file):
        print(f"Error: City file '{city_file}' not found")
        sys.exit(1)

    if not os.path.exists(metromap_file):
        print(f"Error: Metromap file '{metromap_file}' not found")
        sys.exit(1)

    # Create and run visualizer
    visualizer = MetroVisualizer(city_file, metromap_file)
    visualizer.create_visualization()


if __name__ == "__main__":
    main()
