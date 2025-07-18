// Sandbox Simulation - A simple particle simulation using SFML
#include "main.h"
#include <cassert>


// Define the simulation grid size
constexpr int WIDTH = 200;
constexpr int HEIGHT = 150;
constexpr int PIXEL_SIZE = 4; // Screen pixel size per cell

constexpr int FRAME_RATE = 1000; // Max frame rate in FPS
constexpr int TICK_RATE = 30; // Max simulation update rate in Hz
constexpr double TICK_TIME = 1.0 / TICK_RATE; // seconds per tick

constexpr int TEMP_MAX = 3000; // Maximum temperature for particles
constexpr int TEMP_MIN = -273; // Absolute zero, minimum temperature for particles


// Define the types of particles (<= 16)
enum class CellType : uint8_t {
    EMPTY, SAND, WATER, STONE, FIRE, OIL, WOOD, STEAM, SMOKE, ELECTRICITY, GLASS, LAVA, COLD /* 12 */
    // Add up to 16 total
};

enum class InputMode {
    Brush, Debug, Size
    // Add more as needed
};


// Cell data structure
struct Cell {
    CellType type = CellType::EMPTY;
    float density = 0.0f; // Basic density
    int temperature = 0; // For advanced stuff
    int lifetime = 0; // For fire, smoke, etc.
	bool canFall = false; // Does gravity affect this particle?
	int lastUpdate = 0; // For performance optimazations
	bool ParentCell = false; // Is this cell a parent cell? (for fire, etc.)
	// Additional properties can be added here
};

// Simulation class
class Simulation {
public:
    Simulation();

    void handleInput(const sf::RenderWindow& window);
    void update();
    void draw(sf::RenderWindow& window);
	void swapGrids() { std::swap(currentGrid, nextGrid); } // Swap current and next grids

    bool PauseSim = false; // Pause simulation flag
	bool debugMode = false; // Debug mode flag (for temperature, lifetime, etc.)

    InputMode currentMode = InputMode::Brush;
	int modeIndex = 0; // Index for current mode (for hotkeys, etc.)
   

private:

	bool alternateFrame = true;


    std::array<std::array<Cell, WIDTH>, HEIGHT> grid;
    std::array<std::array<Cell, WIDTH>, HEIGHT> grid_updates;

    // Pointers to current/next
    std::array<std::array<Cell, WIDTH>, HEIGHT>* currentGrid = &grid;
    std::array<std::array<Cell, WIDTH>, HEIGHT>* nextGrid = &grid_updates;

    //          -
    //
    // -   directions   +
    //
    //          +

    CellType currentBrush = CellType::SAND; // Default brush type
	int brushSize = 1; // Default brush size

    // Particle logic
    void UpdateWithChecker(int mode);
    void updateCell(int x, int y);
      void updateSand(int x, int y);
      void updateWater(int x, int y);
      void updateFire(int x, int y);
      void updateOil(int x, int y);
      void updateSteam(int x, int y);
      void updateSmoke(int x, int y);
      void updateElectricity(int x, int y);
      void updateGlass(int x, int y);
	  void updateStone(int x, int y);
      void updateWood(int x, int y);
	  void updateLava(int x, int y);
      void updateCold(int x, int y);


    // Utility
    bool inBounds(int x, int y);
    void swapCells(int x1, int y1, int x2, int y2);
    bool canMoveTo(int x, int y, const Cell& from);
    Cell createCell(CellType type, float density, int temp, bool canFall, bool ParentCell);
    std::vector<std::array<int, 2>> getCirclePoints(int cx, int cy, int radius);
    Cell read(int x, int y);
	void write(int x, int y, const Cell& cell);
    void clearCells(int x, int y, int temp);


    // Particle behavior
    void fall(int x, int y);
    void flow(int x, int y);
    void rise(int x, int y);
      void radiateHeat(int x, int y);

    // Misc
    sf::Color getColor(CellType type);
    sf::Color lerpColor(sf::Color a, sf::Color b, float t);
    std::string cellTypeToString(CellType type);
        Cell ClearCell = { CellType::EMPTY, 0.0f, 0, 0, false, 0, 0 }; // Default empty cell

    // Debug
    sf::Color getDebugColor(const Cell& cell);
        sf::Color getTemperatureColor(int temp);
        sf::Color getLifetimeColor(int lifetime);


// Cell properties in an array for easy access and memory density
const Cell cellProperties[13] = {
    //   type, density, temperature, lifetime, canFall, lastUpdate
    {               ClearCell               },
    { CellType::SAND, 1.6f, 20, 0, true, 0 },
    { CellType::WATER, 1.0f, 20, 0, true, 0 },
    { CellType::STONE, 2.5f, 20, 0, false, 0 },
    { CellType::FIRE, 0.003f, 1000, 0, false, 0 },
    { CellType::OIL, 0.825f, 20, 0, true, 0 },
    { CellType::WOOD, 0.75f, 20, 0, false, 0 },
    { CellType::STEAM, 0.0017f, 100, 0, true, 0 },
    { CellType::SMOKE, 0.0029f, 100, 0, true, 0 },
    { CellType::ELECTRICITY, 0.0f, 3000, 0, false, 0 },
    { CellType::GLASS, 2.6f, 1700, 0, false, 0 },
    { CellType::LAVA, 2.7f, 1200, 0, true, 0 },
	{ CellType::COLD, 0.0f, -273, 0, false, 0 } // Cold cell for extreme low temperatures
};
std::unordered_map<CellType, int> PropertyIndexMap = {
    { CellType::EMPTY, 0 },
    { CellType::SAND, 1 },
    { CellType::WATER, 2 },
    { CellType::STONE, 3 },
    { CellType::FIRE, 4 },
    { CellType::OIL, 5 },
    { CellType::WOOD, 6 },
    { CellType::STEAM, 7 },
    { CellType::SMOKE, 8 },
    { CellType::ELECTRICITY, 9 },
    { CellType::GLASS, 10 },
    { CellType::LAVA, 11 },
	{ CellType::COLD, 12 } // Cold cell for extreme low temperatures
};
// Use like this:
// cellProperties[PropertyIndexMap[CellType::TYPE]].value
// Replace TYPE with the desired CellType and value with the property you want to access


};

Simulation::Simulation() {
    // Fill with empty cells
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            write(x, y, ClearCell);
}

bool Simulation::inBounds(int x, int y) {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}

void Simulation::swapCells(int x1, int y1, int x2, int y2) {
    std::swap((*currentGrid)[y1][x1], (*currentGrid)[y2][x2]);

    // Update next grid with the swapped cell
	(*nextGrid)[y1][x1] = (*currentGrid)[y1][x1];
	(*nextGrid)[y2][x2] = (*currentGrid)[y2][x2];

	// Reset last update counter for the swapped cells
	(*nextGrid)[y1][x1].lastUpdate = 0;
	(*nextGrid)[y2][x2].lastUpdate = 0;
}

bool Simulation::canMoveTo(int x, int y, const Cell& from) {
    if (!inBounds(x, y)) return false;
    const Cell& to = read(x, y);
    return (to.type == CellType::EMPTY || to.density < from.density);
}

Cell Simulation::read(int x, int y) {
	return (*currentGrid)[y][x];
}

void Simulation::write(int x, int y, const Cell& cell) {
	(*nextGrid)[y][x] = cell;
}

void Simulation::clearCells(int x, int y, int temp = 0) {
	// Clear the cell at (x, y) in both grids
	(*currentGrid)[y][x] = ClearCell;
	(*nextGrid)[y][x] = ClearCell;
    // Retain temperature
	(*currentGrid)[y][x].temperature = temp;
	(*nextGrid)[y][x].temperature = temp;

}

void Simulation::fall(int x, int y) {

    Cell& cell = read(x, y);
    
    // Do nothing if the cell is not a falling type
    if (!cell.canFall) { write(x, y, cell); return; }

    // DOWNWARD SWAP
    int dxOptions[3] = { 0, -1, 1 }; // down, down-left, down-right
    std::shuffle(std::begin(dxOptions) + 1, std::end(dxOptions), std::default_random_engine(rand())); // randomize diagonals

    for (int i = 0; i < 3; ++i) {
        int nx = x + dxOptions[i];
        int ny = y + 1;

        if (!inBounds(nx, ny)) continue;

        Cell& target = read(nx, ny);

        bool isEmpty = (target.type == CellType::EMPTY);
        bool isLighter = (target.density < cell.density);

        if (isEmpty || isLighter) {
            swapCells(x, y, nx, ny);
            return;
        }
    }
	cell.lastUpdate++; // Increment last update counter if no swap was made

	// If no swap was made, just write the cell back to the next grid
	(*nextGrid)[y][x] = cell; // Write the current cell to the next grid
}

void Simulation::flow(int x, int y) {

    Cell& cell = read(x, y);
    
 
    if (!cell.canFall) { write(x, y, cell); return; }
    

    // Try falling straight down
    if (inBounds(x, y + 1)) {
        Cell& below = read(x, y + 1);
        if (below.density < cell.density) {
            swapCells(x, y, x, y + 1);
            return;
        }
    }

    if (cell.lifetime % 4 != 0) { // update the rest evey 4th tick
        return; // Skip if not time to update
    }

	std::random_device rd; // Random device for seeding
	std::mt19937 rand(rd()); // Random number generator
	std::uniform_int_distribution directions(0, 1); // Random direction (0 or 1)

    
    // Try to flow diagonally down and then horizontally
    for (int dy : {1, 0}) {  // dy = 1 for diagonal down, dy = 0 for horizontal
        bool prevPathBlocked = false;

        for (int dist = 1; dist <= 5; dist++) {

            for (int i = 0; i < 2; i++) {
				int dir = (directions(rand) == 0) ? -1 : 1; // -1 for left, 1 for right
                int newX = x + dist * dir;
                int newY = y + dy;

                if (!inBounds(newX, newY)) continue;

                Cell& targetCell = read(newX, newY);

                if (targetCell.type == CellType::EMPTY || targetCell.density < cell.density) {
                    swapCells(x, y, newX, newY);
                    return;
                }

                if (!prevPathBlocked) {
                    prevPathBlocked = true;
                }
                else {
					prevPathBlocked = false; // Reset once we started moving horizontaly
                    goto OuterLoop;
                }
            }
        }
    OuterLoop:;
    }
	cell.lastUpdate++; // Increment last update counter if no swap was made

	// If no swap was made, just write the cell back to the next grid
	(*nextGrid)[y][x] = cell; // Write the current cell to the next grid
}

void Simulation::rise(int x, int y) {
    
    Cell& cell = read(x, y);

	// Do nothing if the cell is not a rising type
    if (!cell.canFall) { write(x, y, cell); return; }

	if (!inBounds(x, y - 1)) return; // Check bounds

	Cell& above = read(x, y - 1);
	// Check if the cell above is empty or lighter
	if (above.type == CellType::EMPTY || above.density < cell.density) {
		swapCells(x, y, x, y - 1);
		return;
	}
	// Try to rise diagonally
	for (int dx : {-1, 1}) { // left and right
		int nx = x + dx;
		if (inBounds(nx, y - 1)) {
			Cell& diagonalAbove = read(nx, y - 1);
			if (diagonalAbove.type == CellType::EMPTY || diagonalAbove.density < cell.density) {
				swapCells(x, y, nx, y - 1);
				return;
			}
		}
	}
	cell.lastUpdate++; // Increment last update counter if no swap was made

	// If no swap was made, just write the cell back to the next grid
	(*nextGrid)[y][x] = cell; // Write the current cell to the next grid
}

void Simulation::radiateHeat(int x, int y) {
    const int spreadStrength = 8;

    Cell cell = read(x, y); // from current grid
    int selfTemp = cell.temperature;

    int totalDelta = 0;

    // Loop through 8 neighbors
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;

            int nx = x + dx;
            int ny = y + dy;

            if (!inBounds(nx, ny)) continue;

            Cell neighbor = read(nx, ny);
            int neighborTemp = neighbor.temperature;

            // Calculate flow (positive = we give, negative = we receive)
            int flow = (selfTemp - neighborTemp) / spreadStrength;

            if (flow != 0) {
                totalDelta -= flow;

                // Apply to neighbor in nextGrid
                Cell& targetNeighbor = (*nextGrid)[ny][nx];
                targetNeighbor = neighbor;
                targetNeighbor.temperature += flow;
            }
        }
    }

    // Apply net heat change to this cell
    Cell& updated = (*nextGrid)[y][x];
    updated = cell;

    // Apply radiative loss
    int decay = selfTemp / 100;
    updated.temperature = updated.temperature + totalDelta - decay;

    // Clamp to safe range
    updated.temperature = std::clamp(updated.temperature, -273, TEMP_MAX);
}

Cell Simulation::createCell(CellType type, float density, int temp, bool canFall, bool ParentCell) {
    // Create a cell with specified properties (type, density, temperature, lifetime, can Fall?, lastUpdate, Is Parent?)
    return Cell{ type, density, temp, 0, canFall, 0, ParentCell };
}

std::vector<std::array<int, 2>> Simulation::getCirclePoints(int cx, int cy, int radius) {
    std::vector<std::array<int, 2>> result;
    int rSquared = radius * radius;

    for (int dy = -radius; dy <= radius; ++dy) {
        int y = cy + dy;

        // Avoid sqrt for dy^2 > r^2
        int dxLimit = static_cast<int>(std::sqrt(std::max(0, rSquared - dy * dy)));

        for (int dx = -dxLimit; dx <= dxLimit; ++dx) {
            int x = cx + dx;
            result.push_back({ x, y });
        }
    }

    return result;
}

std::string Simulation::cellTypeToString(CellType type) {
    switch (type) {
    case CellType::EMPTY: return "EMPTY";
    case CellType::SAND: return "SAND";
    case CellType::WATER: return "WATER";
    case CellType::STONE: return "STONE";
    case CellType::FIRE: return "FIRE";
    case CellType::OIL: return "OIL";
    case CellType::WOOD: return "WOOD";
    case CellType::STEAM: return "STEAM";
    case CellType::SMOKE: return "SMOKE";
    case CellType::ELECTRICITY: return "ELECTRICITY";
    case CellType::GLASS: return "GLASS";
    default: std::cerr << "cellTypeToString: Type not included!\n"; return "UNKNOWN";
    }
}

void Simulation::handleInput(const sf::RenderWindow& window) {
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        auto mouse = sf::Mouse::getPosition(window);
        int x = mouse.x / PIXEL_SIZE;
        int y = mouse.y / PIXEL_SIZE;

        for (std::array<int, 2> affec : getCirclePoints(x, y, brushSize)) {

            if (inBounds(affec[0], affec[1])) {
                // Write the cell to the current and the next grid
                write(affec[0], affec[1], createCell(currentBrush,
                    cellProperties[PropertyIndexMap[currentBrush]].density,
                    cellProperties[PropertyIndexMap[currentBrush]].temperature,
                    cellProperties[PropertyIndexMap[currentBrush]].canFall, 1));

				(*currentGrid)[affec[1]][affec[0]] = (*nextGrid)[affec[1]][affec[0]]; // Copy to next grid
            }
        }
    }


    // Mode switching
    static bool keyHandled[256] = {}; // Simple key debounce

    auto pressOnce = [&](sf::Keyboard::Key key) {
        if (sf::Keyboard::isKeyPressed(key)) {
            if (!keyHandled[key]) {
                keyHandled[key] = true;
                return true;
            }
        }
        else {
            keyHandled[key] = false;
        }
        return false;
    };

    // Mode hotkeys
    if (pressOnce(sf::Keyboard::D)) currentMode = InputMode::Debug;
    if (pressOnce(sf::Keyboard::B)) currentMode = InputMode::Brush;
    if (pressOnce(sf::Keyboard::S)) currentMode = InputMode::Size;
    

    switch (currentMode) {
    case InputMode::Debug: // Debug mode
        if (pressOnce(sf::Keyboard::Num1)) {
            std::cout << "Debug: Temperature View\n";
			modeIndex = 1; // Set mode index for temperature view
        }
        if (pressOnce(sf::Keyboard::Num2)) {
            std::cout << "Debug: Lifetime View\n";
			modeIndex = 2; // Set mode index for lifetime view
        }

		if (pressOnce(sf::Keyboard::Num0)) {
			std::cout << "Debug: Default View\n";
			modeIndex = 0; // Set mode index for default view
		}
        break;

    case InputMode::Brush: // Brush drawing elements mode
        if (pressOnce(sf::Keyboard::Num1)) {
			currentBrush = CellType::SAND;
        }
		if (pressOnce(sf::Keyboard::Num2)) {
			currentBrush = CellType::WATER; // Set brush to water
		}
		if (pressOnce(sf::Keyboard::Num3)) {
			currentBrush = CellType::STONE; // Set brush to stone
		}
		if (pressOnce(sf::Keyboard::Num4)) {
			currentBrush = CellType::FIRE; // Set brush to fire
		}
		if (pressOnce(sf::Keyboard::Num5)) {
			currentBrush = CellType::OIL; // Set brush to oil
		}
		if (pressOnce(sf::Keyboard::Num6)) {
			currentBrush = CellType::WOOD; // Set brush to wood
		}
		if (pressOnce(sf::Keyboard::Num7)) {
			currentBrush = CellType::STEAM; // Set brush to steam
		}
		if (pressOnce(sf::Keyboard::Num8)) {
			currentBrush = CellType::COLD; // Set brush to smoke
		}
		if (pressOnce(sf::Keyboard::Num9)) {
			currentBrush = CellType::ELECTRICITY; // Set brush to electricity
		}
        if (pressOnce(sf::Keyboard::Num0)) {
            currentBrush = CellType::EMPTY; // Set brush to empty
        }
        break;

	case InputMode::Size: // Size changing mode
        
        if (pressOnce(sf::Keyboard::Add) || pressOnce(sf::Keyboard::Equal)) {
            // Increase brush size
			std::cerr << "Increased brush size\n";
			brushSize++;
        }
        if (pressOnce(sf::Keyboard::Subtract) || pressOnce(sf::Keyboard::Hyphen)) {
            // Decrease brush size
			std::cerr << "Decreased brush size\n";
			if (brushSize > 1) brushSize--; // Prevent negative size
        }
        break;

    }
    // Add more modes as needed


}

void Simulation::UpdateWithChecker(int mode) {

    if (mode == 0) {
        // Update the next grid based on the current grid
        for (int y = HEIGHT - 1; y >= 0; --y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (x % 2 == 1) {
                    radiateHeat(x, y); // Radiate heat
                    updateCell(x, y);
                }
            }
        }
	}
    else if (mode == 1) {
        // Update the next grid based on the current grid
        for (int y = HEIGHT - 1; y >= 0; --y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (x % 2 == 0) {
                    radiateHeat(x, y); // Radiate heat
                    updateCell(x, y);
                }
            }
        }
    }
	else if (mode == 2) {
		// Update the next grid based on the current grid
		for (int y = HEIGHT - 1; y >= 0; --y) {
			for (int x = WIDTH - 1; x >= 0; --x) {
				if (x % 2 == 1) {
					radiateHeat(x, y); // Radiate heat
					updateCell(x, y);
				}
			}
		}
	}
	else if (mode  == 3) {
		// Update the next grid based on the current grid
        for (int y = HEIGHT - 1; y >= 0; --y) {
			for (int x = WIDTH - 1; x >= 0; --x) {
				if (x % 2 == 0) {
					radiateHeat(x, y); // Radiate heat
					updateCell(x, y);
				}
			}
		}
	}
    assert(mode < 4);
    
}

void Simulation::update() {    

    // Update the grid with a checker pattern
    if (alternateFrame) {
        // Update with checker pattern
		UpdateWithChecker(0); 
		UpdateWithChecker(1);

		alternateFrame = false; // Toggle for next frame
	}
	else {
		// Update without checker pattern
        UpdateWithChecker(2);
		UpdateWithChecker(3);

        alternateFrame = true; // Toggle for next frame
	}

	std::swap(currentGrid, nextGrid); // Swap the grids to apply updates



}

void Simulation::updateCell(int x, int y) {
    switch (read(x, y).type) {
	case CellType::EMPTY: break; // Do nothing for empty cells
    case CellType::SAND: updateSand(x, y); break;
    case CellType::WATER: updateWater(x, y); break;
    case CellType::STONE: updateStone(x, y); break;
	case CellType::WOOD: updateWood(x, y); break;
    case CellType::FIRE: updateFire(x, y); break;
    case CellType::OIL: updateOil(x, y); break;
    case CellType::STEAM: updateSteam(x, y); break;
    case CellType::SMOKE: updateSmoke(x, y); break;
    case CellType::ELECTRICITY: updateElectricity(x, y); break;
    case CellType::GLASS: updateGlass(x, y); break;
	case CellType::LAVA: updateLava(x, y); break;
    case CellType::COLD: updateCold(x, y); break;

    default: std::cerr << "In function updateCell - Type " << cellTypeToString(grid[y][x].type) << " not included!\n"; break;
    }
}

void Simulation::updateSand(int x, int y) {

    Cell& cell = read(x, y);

    cell.lifetime++;
	

    if (cell.lastUpdate >= 300) {
		if (cell.lifetime % 90 != 0) return; // Update less if the cell has not been updated recently
    }
	else if (cell.lastUpdate >= 60) {
        if (cell.lifetime % 30 != 0) return; // Update less if the cell has not been updated recently
	}

    if (cell.temperature >= 1700) (*nextGrid)[y][x].type = CellType::GLASS;
    
    
    fall(x, y); 

}

void Simulation::updateWater(int x, int y) {

    Cell& cell = read(x, y);

    cell.lifetime++;


    if (cell.lastUpdate >= 60 && cell.lifetime % TICK_RATE / 3 != 0) return; // Update less if the cell has not been updated recently
    
	if (cell.temperature >= 100) {
		// Convert water to steam if it reaches boiling point
        (*nextGrid)[y][x].type = CellType::STEAM;
		// Set temperature to steam's temperature
        (*nextGrid)[y][x].temperature = cellProperties[PropertyIndexMap[CellType::STEAM]].temperature + 3;
	}
    

    flow(x, y);
}

void Simulation::updateFire(int x, int y) {
	constexpr int MAX_FIRE_HEIGHT = 6; // Maximum height of fire

    Cell& cell = read(x, y);

    cell.lifetime++;


    int maxLife = cell.ParentCell ? 90 : 40;
    if (cell.lifetime >= maxLife + (rand() % 5)) {
		clearCells(x, y, cell.temperature); // Clear the cell if it has lived too long
        return;
    }

    int r = rand();
    int maxHeight = std::max(1, MAX_FIRE_HEIGHT - (cell.lifetime / 10));

    // Only allow spreading if random roll + age is below a threshold
    if ((r % 100) > 80 || cell.lifetime > 40) return; // Old fire spreads less

    // Try only 1-2 cells upward, not the full MAX_FIRE_HEIGHT
    int maxSpreadHeight = 1 + (r % 2); // Try to go 1 or 2 cells up

    float spreadChance = 1.0f - (float)cell.lifetime / 50.0f;
    if ((rand() % 100) > (spreadChance * 100)) return; // Spreads less over time

    for (int i = 1; i <= maxSpreadHeight; ++i) {

        int fireNeighbors = 0;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;
                if (inBounds(nx, ny) && read(nx, ny).type == CellType::FIRE) fireNeighbors++;
            }
        }

        if (fireNeighbors >= 6) clearCells(x, y); return; // Too much fire nearby, skip spreading


        int dx = (rand() % 3) - 1; // -1, 0, or 1
        int nx = x + dx;
        int ny = y - i;

        if (!inBounds(nx, ny)) continue;
        Cell& targetCell = read(nx, ny);

        if (targetCell.type == CellType::EMPTY || targetCell.density < cell.density) {
            // Slightly vary new cell lifetime to avoid synchronized death
            int newLifetime = cell.lifetime + 1 + (rand() % 3);

            write(nx, ny, createCell(
                CellType::FIRE,
                cellProperties[PropertyIndexMap[CellType::FIRE]].density,
                cellProperties[PropertyIndexMap[CellType::FIRE]].temperature,
                false,
                0
            ));
            (*nextGrid)[ny][nx].lifetime = newLifetime;
            break; // Only spread to one cell this frame
        }
    }

	write(x, y, cell); // Write the current cell to the next grid
}

void Simulation::updateOil(int x, int y) {

    Cell& cell = read(x, y);

    cell.lifetime++;


}

void Simulation::updateSteam(int x, int y) {

    Cell& cell = read(x, y);

	cell.lifetime++;


	if (cell.temperature <= 100) (*nextGrid)[y][x].type = CellType::WATER; // Convert steam back to water if it cools down

	rise(x, y); // Steam rises by default
}

void Simulation::updateSmoke(int x, int y) {

    Cell& cell = read(x, y);

    cell.lifetime++;


	if (cell.lifetime > (600 + rand() % 100)) { // Smoke dissipates after a while
        (*nextGrid)[y][x].type = CellType::EMPTY;
		return;
	}

    rise(x, y);
}

void Simulation::updateElectricity(int x, int y) {

    Cell& cell = read(x, y);

    cell.lifetime++;
    if (cell.lifetime >= 8) {
        clearCells(x, y, cell.temperature);
        return;
    }

    // Count neighbors
    int totalDx = 0;
    int totalDy = 0;
    int neighborCount = 0;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (inBounds(nx, ny) && read(nx, ny).type == CellType::ELECTRICITY) {
                totalDx += dx;
                totalDy += dy;
                neighborCount++;
            }
        }
    }

    if (neighborCount < 1 || neighborCount > 5) {
        if (rand() % 2 == 0) {
            clearCells(x, y, cell.temperature);
            return;
        }
    }

    // Spread chance drops over time
    float spreadChance = 1.0f - (float)cell.lifetime / 8.0f;
    if ((rand() % 100) >= (int)(spreadChance * 100)) {
        write(x, y, cell);
        return;
    }

    int biasX = -totalDx;
    int biasY = -totalDy;

    int finalDx = biasX + (rand() % 3 - 1);
    int finalDy = biasY + (rand() % 3 - 1);
    finalDx = std::clamp(finalDx, -1, 1);
    finalDy = std::clamp(finalDy, -1, 1);

    if (finalDx != 0 || finalDy != 0) {
        int nx = x + finalDx;
        int ny = y + finalDy;
        if (inBounds(nx, ny) && read(nx, ny).type == CellType::EMPTY) {
            write(nx, ny, createCell(CellType::ELECTRICITY, 0.0f, cellProperties[PropertyIndexMap[CellType::ELECTRICITY]].temperature, false, false));
            (*nextGrid)[ny][nx].lifetime = cell.lifetime + 1;
        }
    }

	// Write the current cell back to the next grid
    write(x, y, cell);

}

void Simulation::updateGlass(int x, int y) {

    Cell& cell = read(x, y);

    fall(x, y);

    cell.lifetime++;
}

void Simulation::updateStone(int x, int y) {

    Cell& cell = read(x, y);

    if (cell.temperature >= 1200) (*nextGrid)[y][x].type = CellType::LAVA;

    fall(x, y);

	cell.lifetime++;
}

void Simulation::updateWood(int x, int y) {

    Cell& cell = read(x, y);

    fall(x, y);

    cell.lifetime++;
}

void Simulation::updateLava(int x, int y) {

    Cell& cell = read(x, y);

    if (cell.temperature <= 1200) (*nextGrid)[y][x].type = CellType::STONE;

    flow(x, y);

    cell.lifetime++;
}

void Simulation::updateCold(int x, int y) {

    Cell& cell = read(x, y);

	clearCells(x, y, cell.temperature); // Clear the cell but retain temperature
	// Cold cells do not move or interact, just make thing really cold for some time

}

sf::Color Simulation::getDebugColor(const Cell& cell) {

    if (modeIndex == 1) {
        return getTemperatureColor(cell.temperature);
    }
	if (modeIndex == 2) {
        return getLifetimeColor(cell.lifetime);
	}

	if (modeIndex == 0) {
		// Default view, return color based on cell type
		return getColor(cell.type);
	}

	// Fallback color if no mode matches
	return sf::Color::Magenta; // Magenta color
}

sf::Color Simulation::lerpColor(sf::Color a, sf::Color b, float t) {
    return sf::Color(
        static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
        static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
        static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
        static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
    );
}

sf::Color Simulation::getLifetimeColor(int lifetime) {
	
    return sf::Color(30, 30, 30, 255);
}

sf::Color Simulation::getTemperatureColor(int temp) {

    // Handle out of bounds temperatures
    if (temp <= -273) return sf::Color(255, 255, 255, 30);  // White, very transparent

    // Handle extreme low temperatures
    if (temp < -100) {
        float t = (temp + 273.0f) / 173.0f;
        return lerpColor(sf::Color(255, 255, 255, 30), sf::Color(45, 90, 170, 60), t);
    }
    // Handle temperatures below 0°C
    if (temp < 0) {
        float t = (temp + 100.0f) / 100.0f;
        return lerpColor(sf::Color(45, 90, 170, 60), sf::Color(0, 0, 0, 100), t);
    }
    // Handle temperatures from 0°C to 30°C
    if (temp <= 30) {
        float t = temp / 30.0f;
        return lerpColor(sf::Color(0, 0, 0, 80), sf::Color(40, 40, 255, 110), t);
    }
    // Handle temperatures from 30°C to 60°C
    if (temp <= 60) {
        float t = (temp - 30) / 30.0f;
        return lerpColor(sf::Color(40, 40, 255, 110), sf::Color(255, 150, 0, 160), t);
    }
    // Handle temperatures from 60°C to 100°C
    if (temp <= 100) {
        float t = (temp - 60) / 50.0f; // starts from 60°C to 110°C
        return lerpColor(sf::Color(255, 150, 0, 160), sf::Color(255, 50, 0, 200), t);
    }
    // 100 to 500: goes from red to orange
    if (temp <= 500) {
        float t = (temp - 100) / 400.0f;
        return lerpColor(sf::Color(255, 0, 0, 180), sf::Color(255, 160, 0, 230), t);
    }
    // 100 to 500, but reversers the order to go from orange to red
    if (temp <= 1000) {
        float t = (temp - 500) / 500.0f;
        return lerpColor(sf::Color(255, 160, 0, 230), sf::Color(255, 0, 0, 255), t);
    }
    // Handle 1000 to TEMP_MAX with a 3-point gradient
    if (temp <= TEMP_MAX) {
        float t = (temp - 1000) / float(TEMP_MAX - 1000);

        if (t < 0.5f) {
            // 1000 → midpoint (e.g., 2000): red → orange
            float localT = t / 0.5f;
            return lerpColor(sf::Color(255, 0, 0, 255), sf::Color(255, 100, 0, 255), localT);
        }
        else {
            // midpoint → TEMP_MAX: orange → white
            float localT = (t - 0.5f) / 0.5f;
            return lerpColor(sf::Color(255, 100, 0, 255), sf::Color(255, 255, 255, 255), localT);
        }
    }


    // Beyond TEMP_MAX, return white
    return sf::Color(255, 255, 255);
}


void Simulation::draw(sf::RenderWindow& window) {
    sf::VertexArray cells(sf::Quads);

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const Cell& cell = read(x, y);


            float px = x * PIXEL_SIZE;
            float py = y * PIXEL_SIZE;
            

            sf::Color color;

            if (currentMode == InputMode::Debug) {
                color = getDebugColor(cell);
            }
            else {
                color = getColor(cell.type);
            }

			// Add the cell as a quad
            cells.append(sf::Vertex(sf::Vector2f(px, py), color));
            cells.append(sf::Vertex(sf::Vector2f(px + PIXEL_SIZE, py), color));
            cells.append(sf::Vertex(sf::Vector2f(px + PIXEL_SIZE, py + PIXEL_SIZE), color));
            cells.append(sf::Vertex(sf::Vector2f(px, py + PIXEL_SIZE), color));
        }
    }

    window.draw(cells);
}

sf::Color Simulation::getColor(CellType type) {
    switch (type) {
    case CellType::EMPTY: return sf::Color(30, 30, 30, 255);
    case CellType::SAND: return sf::Color(194, 178, 128, 255);
    case CellType::WATER: return sf::Color(50, 100, 255, 255);
    case CellType::STONE: return sf::Color(100, 100, 100, 255);
    case CellType::FIRE: return sf::Color(255, 80, 20, 255);
    case CellType::WOOD: return sf::Color(85, 45, 15, 255);
    case CellType::OIL: return sf::Color(50, 40, 40, 255);
    case CellType::STEAM: return sf::Color(190, 180, 180, 255);
    case CellType::SMOKE: return sf::Color(50, 45, 45, 255);
    case CellType::ELECTRICITY: return sf::Color(0, 230, 250, 255);
    case CellType::GLASS: return sf::Color(185, 225, 230, 255);
	case CellType::LAVA: return sf::Color(255, 100, 0, 255);
	case CellType::COLD: return sf::Color(207, 207, 247, 150); // Cold cells
    default: std::cerr << "getColor: Type not included!\n"; return sf::Color::Magenta;
    }
}

int main() {

    srand(time(NULL));

    // Move large objects to the heap  
    auto window = std::make_unique<sf::RenderWindow>(sf::VideoMode(WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE), "Sandbox");
    window->setFramerateLimit(FRAME_RATE);

    auto sim = std::make_unique<Simulation>();

    auto previous = std::chrono::high_resolution_clock::now();
    double lag = 0.0;

	std::ofstream File("log.txt");
    
    int i = 0;
	float lastFPS = TICK_RATE;

	// Main loop
    while (window->isOpen()) {

        auto current = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = current - previous;
        previous = current;

        if (!sim->PauseSim) lag += elapsed.count();

        // Always handle window events
        sf::Event event;
        while (window->pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window->close();
        }


        sim->handleInput(*window);


        // Only update simulation if not paused
        while (lag >= TICK_TIME) {
            if (!sim->PauseSim) {
                sim->update(); // Updates only when unpaused
                lag -= TICK_TIME;
            }
            else {
                sim->swapGrids(); // Swap grids after each tick
                break; // Exit the loop if paused
            }
        }


        // Always render — even when paused
        window->clear();
        sim->draw(*window);
        window->display();

        // Optional FPS logging
        auto end = std::chrono::high_resolution_clock::now();

        // Calculate and display FPS every 10 frames
        if (i % static_cast<int>(lastFPS) == 0) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - current).count();
			lastFPS = 1000000.f / duration;
            std::cout << "FPS: " << lastFPS << '\n';
			i = 0; // Reset frame counter
        }
        
        i++;
    }


   return 0;  
}
