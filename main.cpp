// Sandbox Simulation - A simple particle simulation using SFML
#include "main.h"


// Define the simulation grid size
constexpr int WIDTH = 200;
constexpr int HEIGHT = 150;
constexpr int PIXEL_SIZE = 4; // Screen pixel size per cell

constexpr int FRAME_RATE = 0; // Max frame rate in FPS
constexpr int TICK_RATE = 30; // Max simulation update rate in Hz
constexpr double TICK_TIME = 1.0 / TICK_RATE; // seconds per tick

constexpr int TEMP_MAX = 3000; // Maximum temperature for particles
constexpr int TEMP_MIN = -273; // Absolute zero, minimum temperature for particles


// global frame counter for simulation updates
int frameCounter = 0;

// Define the types of particles (<= 16)
enum class CellType : uint8_t {
    EMPTY = 0, SAND = 1, WATER = 2, STONE = 3, FIRE = 4, OIL = 5, WOOD = 6, STEAM = 7, SMOKE = 8, ELECTRICITY = 9, GLASS = 10, LAVA = 11, COLD = 12 /* 13 (starts at 0) */
    // Add up to 16 total
};
constexpr uint8_t CURRENT_TYPE_AMOUNT = 13;

enum class Category : uint8_t {
    SOLID = 0,
    LIQUID = 1,
    GAS = 2,
    OTHER = 3
};

enum class InputMode {
    Brush, Debug, Size
    // Add more as needed
};

#pragma pack(push, 1) // force tight layout
struct SolidData {
    uint8_t canFall : 1;
    uint8_t reserved : 7; // room for more flags later
};
struct LiquidData {
    uint8_t density : 4;  // 0–15 is enough for all fluids
    uint8_t reserved : 4; // unused or flag space
};
struct GasData {
    uint8_t lifetime;
    // more can go here
};
struct OtherData {
    uint8_t parentCell : 1;
    uint8_t reserved : 3;
    uint8_t lifetime : 4; // capped at 15 ticks (2^4 - 1)
};


union CellData {
    SolidData solid;
    LiquidData liquid;
    GasData gas;
    OtherData other;
};

struct Cell {
    // 1 byte: 4-bit type + 4-bit general flags
    uint8_t type : 4;
    uint8_t flags : 4;

    // 1 byte: general-purpose update marker
    uint8_t lastUpdate = 0;

    // 13 bits: temperature (-4095 to 4095) 
    int16_t temperature : 13;
    int16_t category : 3; // 3 bits for category (max 8 categories)


    // 1 byte: low precision thermal conductivity (0 to 255)
    uint8_t thermalConductivity = 0;

    // 1 byte: optional (e.g., a local ID or additional global flag byte)
    // uint8_t extraField = 0;

    // 4 bytes: union
    CellData data;
};
#pragma pack(pop)

struct CellProperties {
    CellType type;
    int16_t temperature;
    uint8_t density;
    bool canFall;
	uint8_t thermalConductivity; // 0 = no heat move through, 255 = all heat moves through
};



// Simulation class
class Simulation {
public:
    Simulation();

    void handleInput(const sf::RenderWindow& window);
    void update();
    void draw(sf::RenderWindow& window);
    void swapGrids() { std::swap(currentGrid, nextGrid); } // Swap current and next grids

    void Simulation::drawDebugText(sf::RenderWindow& window);


    bool PauseSim = false; // Pause simulation flag
    bool debugMode = false; // Debug mode flag (for temperature, lifetime, etc.)

    InputMode currentMode = InputMode::Brush;
    int debugModeIndex = 0; // Index for current mode (for hotkeys, etc.)
	bool debugText = false; // Show debug text flag


private:

    int alternatingFrames = 0;


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
    void UpdateWithChecker(int mode, bool odd);
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
    // bool canMoveTo(int x, int y, const Cell& from);
    Category getCategoryFromType(CellType type);
    CellType getCellType(const Cell& c);
    Cell createCell(CellType type, int16_t temperature, int8_t density, bool canFall);

    std::vector<std::array<int, 2>> getCirclePoints(int cx, int cy, int radius);
    Cell& read(int x, int y);
    void write(int x, int y, const Cell& cell);
    void clearCells(int x, int y, float temp);


    // Particle behavior
    void fall(int x, int y);
    void flow(int x, int y);
    void rise(int x, int y);
    void radiateHeat(int x, int y);

    // Misc.
    sf::Color getColor(CellType type);
    sf::Color lerpColor(sf::Color a, sf::Color b, float t);
    std::string cellTypeToString(CellType type);

    // Debug
    sf::Color getDebugColor(const Cell& cell);
    sf::Color getTemperatureColor(float temp);
    sf::Color getLowTemperatureColor(float temp);
    sf::Color getLifetimeColor(int lifetime);


    const CellProperties cellProperties[CURRENT_TYPE_AMOUNT] = {
        //    Type               Temp   Dens  Fall  ThermalConductivity
        { CellType::EMPTY,        20,    0,  false,  0 }, // vacuum/air
        { CellType::SAND,         20,    2,  true,   0 },
        { CellType::WATER,        20,    1,  true,   0 },
        { CellType::STONE,        20,    3,  false,  0 },
        { CellType::FIRE,       1000,    0,  true,   0 },
        { CellType::OIL,          20,    1,  true,   0 },
        { CellType::WOOD,         20,    1,  false,  0 },
        { CellType::STEAM,       100,    0,  true,   0 },
        { CellType::SMOKE,       100,    0,  true,   0 },
        { CellType::ELECTRICITY,3000,    0,  false,  0 },
        { CellType::GLASS,      1700,    3,  false,  0 },
        { CellType::LAVA,       1200,    3,  false,  0 },
        { CellType::COLD,       -273,    0,  false,  0 }
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
    

};

Simulation::Simulation() {
    // Fill with empty cells
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            write(x, y, createCell(CellType::EMPTY, 0, 0, false));
        }
    }

}

bool Simulation::inBounds(int x, int y) {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}

CellType Simulation::getCellType(const Cell& c) {
    if (c.type > CURRENT_TYPE_AMOUNT) {
        std::cerr << "Invalid cell type: " << static_cast<int>(c.type) << "\n";
        assert(false && "Invalid cell type");
    }
    return static_cast<CellType>(c.type);
}



inline uint8_t toRawType(CellType t) {
    return static_cast<uint8_t>(t);
}

inline bool getCanFall(const Cell& cell) {
    return cell.category == 0 ? cell.data.solid.canFall : false;
}

inline int getDensity(const Cell& cell) {
    return cell.category == 1 ? (cell.data.liquid.density / 4.0f) : 0.0f;
    // You can change the divisor based on how you encode density (scaling from 0–15)
}

inline bool isParentCell(const Cell& cell) {
    return cell.category == 3 ? cell.data.other.parentCell : false;
}

inline uint8_t getLifetime(const Cell& cell) {
    switch (cell.category) {
    case 2: return cell.data.gas.lifetime;
    case 3: return cell.data.other.lifetime;
    default: return 0;
    }
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

Cell& Simulation::read(int x, int y) {
    return (*currentGrid)[y][x];
}

void Simulation::write(int x, int y, const Cell& cell) {
    // Write the cell to the next grid
    (*nextGrid)[y][x] = cell;
}

void Simulation::clearCells(int x, int y, float temp = 0.f) {
    // Clear the cell at (x, y) in both grids
    (*currentGrid)[y][x] = createCell(CellType::EMPTY, 0, 0, 0);
    (*nextGrid)[y][x] = (*currentGrid)[y][x];
    // Retain temperature
    (*currentGrid)[y][x].temperature = temp;
    (*nextGrid)[y][x].temperature = temp;

}

void Simulation::fall(int x, int y) {

    Cell& cell = read(x, y);

    // Do nothing if the cell is not a falling type
    if (!cell.data.solid.canFall) { write(x, y, cell); return; }

    // DOWNWARD SWAP
    int dxOptions[3] = { 0, -1, 1 }; // down, down-left, down-right
    std::shuffle(std::begin(dxOptions) + 1, std::end(dxOptions), std::default_random_engine(rand())); // randomize diagonals

    for (int i = 0; i < 3; ++i) {
        int nx = x + dxOptions[i];
        int ny = y + 1;

        if (!inBounds(nx, ny)) continue;

        Cell& target = read(nx, ny);
        
        if (getCellType(target) == CellType::EMPTY) {
            swapCells(x, y, nx, ny);
            return;
        }
    }
    if (cell.lastUpdate < 256) {
        cell.lastUpdate++; // Increment last update counter if no swap was made
    }

    // If no swap was made, just write the cell back to the next grid
    (*nextGrid)[y][x] = cell; // Write the current cell to the next grid
}

void Simulation::flow(int x, int y) {
    Cell& cell = read(x, y);

    // Try falling straight down
    if (inBounds(x, y + 1)) {
        Cell& below = read(x, y + 1);
        if (below.data.liquid.density < cell.data.liquid.density || below.type == toRawType(CellType::EMPTY)) {
            swapCells(x, y, x, y + 1);
            return;
        }
    }


    // Use static RNG for performance
    static thread_local std::mt19937 rng(std::random_device{}());

    // Try diagonal down first (dy = 1), then horizontal (dy = 0)
    for (int dy : {1, 0}) {
        std::array<int, 2> directions = { -1, 1 }; // left, right
        std::shuffle(directions.begin(), directions.end(), rng);

        for (int dir : directions) {
            for (int dist = 1; dist <= 5; ++dist) {
                int newX = x + dist * dir;
                int newY = y + dy;

                if (!inBounds(newX, newY)) continue;

                Cell& target = read(newX, newY);
                
                if (getCellType(target) == CellType::EMPTY || target.data.liquid.density < cell.data.liquid.density) {
                    swapCells(x, y, newX, newY);
                    return;
                }
                
                // Break out early if diagonal blocked; don't keep checking horizontally in that direction
                if (dy == 1) break;
            }
        }
    }

    // No movement occurred — just store the cell back
    cell.lastUpdate++;
    (*nextGrid)[y][x] = cell;

}

void Simulation::rise(int x, int y) {

    Cell& cell = read(x, y);


    if (!inBounds(x, y - 1)) return; // Check bounds

    Cell& above = read(x, y - 1);
    // Check if the cell above is empty or lighter
    if (getCellType(above) == CellType::EMPTY) {
        swapCells(x, y, x, y - 1);
        return;
    }

    // Try to rise diagonally
    for (int dx : {-1, 1}) { // left and right
        int nx = x + dx;
        if (inBounds(nx, y - 1)) {
            Cell& diagonalAbove = read(nx, y - 1);
            if (getCellType(diagonalAbove) == CellType::EMPTY) {
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
    float selfTemp = read(x, y).temperature;
    float netDelta = 0.0f;

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;

            int nx = x + dx;
            int ny = y + dy;
            if (!inBounds(nx, ny)) continue;

            float neighborTemp = read(nx, ny).temperature;

            float diff = (selfTemp - neighborTemp) * 0.01f;
            if (std::abs(diff) < 0.001f) continue;

            netDelta -= diff;
            (*nextGrid)[ny][nx].temperature += diff;
            (*nextGrid)[ny][nx].temperature = std::clamp((*nextGrid)[ny][nx].temperature, static_cast<int16_t>(TEMP_MIN), static_cast<int16_t>(TEMP_MAX));
        }
    }

    Cell newCell = read(x, y);
    newCell.temperature += netDelta;
    newCell.temperature = std::clamp(newCell.temperature, static_cast<int16_t>(TEMP_MIN), static_cast<int16_t>(TEMP_MAX));
    (*nextGrid)[y][x] = newCell;
}

Category Simulation::getCategoryFromType(CellType type) {
    switch (type) {
    case CellType::SAND:
    case CellType::STONE:
    case CellType::WOOD:
    case CellType::GLASS:
        return Category::SOLID;

    case CellType::WATER:
    case CellType::OIL:
    case CellType::LAVA:
        return Category::LIQUID;
    
    case CellType::STEAM:
    case CellType::SMOKE:
		return Category::GAS;

    case CellType::FIRE:
    case CellType::ELECTRICITY:
    case CellType::COLD:
    case CellType::EMPTY:
        return Category::OTHER;

    default:
        assert(false && "Invalid CellType passed to getCategoryFromType");
    }
}

Cell Simulation::createCell(CellType type, int16_t temperature, int8_t density, bool canFall) {
    // Make sure that the values are correct using assertions, since they won't be checked in release builds
    assert(static_cast<uint8_t>(type) < CURRENT_TYPE_AMOUNT && "createCell: Invalid CellType\n");
    assert(temperature >= TEMP_MIN && temperature <= TEMP_MAX && "createCell: Temperature out of bounds\n");
	assert(density >= 0 && density <= 15 && "createCell: Density out of bounds\n");
	assert(canFall == true || canFall == false && "createCell: canFall must be true or false.\n");
    // Now we know that all of the values are correct.
    

    // Make a cell skeleton to which we add the properties
    Cell c = {};

	// Add the properties to the cell
	c.type = toRawType(type);
	c.flags = 0b0000; // Reset flags
	c.lastUpdate = 0; // Reset last update counter
    c.temperature = temperature;
	c.category = static_cast<uint8_t>(getCategoryFromType(type));
	c.thermalConductivity = cellProperties[PropertyIndexMap[type]].thermalConductivity; // Set thermal conductivity based on the property array
	// All of the properties are set, now we can add the data union

	switch (getCategoryFromType(type)) {
	  case Category::SOLID:
		  c.data.solid.canFall = canFall ? 1 : 0; // Set canFall flag
		  break; // Break after setting solid data
	  case Category::LIQUID:
		  c.data.liquid.density = density; // Set density (0-15)
		  break; // Break after setting liquid data
	  case Category::GAS:
		  c.data.gas.lifetime = 0; // Set lifetime to 0
		  break; // Break after setting gas data
	  case Category::OTHER:
		  c.data.other.parentCell = 0; // Set parent cell flag to false. Can be set true after function call
		  c.data.other.lifetime = 0; // Set lifetime to 0
		  break; // Break after setting other data
	default:
		// Just-in-case fallback. Should never be reached.
		assert(false && "createCell: Invalid CellType passed to createCell function!"); // Invalid type
	}

	return c;
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
	case CellType::LAVA: return "LAVA";
	case CellType::COLD: return "COLD";

    default: std::cerr << "cellTypeToString: Type (int)" << static_cast<int>(type) << " not included!\n"; return "UNKNOWN";
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
                const CellProperties& base = cellProperties[PropertyIndexMap[currentBrush]];

                write( affec[0], affec[1],
                    createCell( currentBrush, base.density, base.temperature, base.canFall ));

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
            debugModeIndex = 1; // Set mode index for temperature view
        }
        if (pressOnce(sf::Keyboard::Num2)) {
            std::cout << "Debug: Low Temperature View\n";
            debugModeIndex = 2; // Set mode index for second temperature view
        }
		if (pressOnce(sf::Keyboard::Num3)) {
			std::cout << "Debug: Lifetime View\n";
			debugModeIndex = 3; // Set mode index for lifetime view
		}

        if (pressOnce(sf::Keyboard::Num9)) {

            debugText = !debugText; // Toggle debug text display
            if (debugText) {
                std::cout << "Debug: Debug Text Enabled\n";
            }
            else {
                std::cout << "Debug: Debug Text Disabled\n";
            }
        }
        if (pressOnce(sf::Keyboard::Num0)) {
            std::cout << "Debug: Default View\n";
            debugModeIndex = 0; // Set mode index for default view
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
    }
    // Add more modes as needed
    
	// Miscellaneous controls
    if (pressOnce(sf::Keyboard::Add) || pressOnce(sf::Keyboard::Equal)) {
        // Increase brush size
        brushSize++;
        std::cerr << "Increased brush size to " << brushSize << '\n';
    }
    if (pressOnce(sf::Keyboard::Subtract) || pressOnce(sf::Keyboard::Hyphen)) {
        // Decrease brush size
        if (brushSize > 1) brushSize--; // Prevent negative size
        std::cerr << "Decreased brush size to " << brushSize << '\n';
    }
	if (pressOnce(sf::Keyboard::Space)) {
		// Toggle simulation pause
		PauseSim = !PauseSim;
		std::cerr << "Simulation " << (PauseSim ? "paused" : "resumed") << '\n';
	}
    



}

void Simulation::UpdateWithChecker(int mode, bool odd) {

    int xStart, xEnd, xStep = 0;
    int yStart, yEnd, yStep = 0;

    switch (mode) {
    case 0: // Top to Bottom, Left to Right
        yStart = 0;        yEnd = HEIGHT; yStep = 1;
        xStart = 0;        xEnd = WIDTH;  xStep = 1;
        break;
    case 1: // Top to Bottom, Right to Left
        yStart = 0;        yEnd = HEIGHT; yStep = 1;
        xStart = WIDTH - 1;  xEnd = -1;     xStep = -1;
        break;
    case 2: // Bottom to Top, Left to Right
        yStart = HEIGHT - 1; yEnd = -1;     yStep = -1;
        xStart = 0;        xEnd = WIDTH;  xStep = 1;
        break;
    case 3: // Bottom to Top, Right to Left
        yStart = HEIGHT - 1; yEnd = -1;     yStep = -1;
        xStart = WIDTH - 1;  xEnd = -1;     xStep = -1;
        break;
    default:
        assert(false && "function UpdateWithChecker mode invalid!"); // Invalid mode
    }

    for (int y = yStart; y != yEnd; y += yStep) {
        for (int x = xStart; x != xEnd; x += xStep) {
            if ((x % 2 == 1) == odd) {
                radiateHeat(x, y);
                updateCell(x, y);
            }
        }
    }


}

void Simulation::update() {

    // Update the grid with a checker pattern and alternating frame updating direction
    if (alternatingFrames == 0) {
        // Update with checker pattern
        UpdateWithChecker(0, 0);
        UpdateWithChecker(1, 1);

        alternatingFrames++; // Increment for next frame
    }
    else if (alternatingFrames == 1) {
        // Update without checker pattern
        UpdateWithChecker(1, 1);
        UpdateWithChecker(2, 0);

        alternatingFrames++; // Increment for next frame
    }
    else if (alternatingFrames == 2) {
        // Update without checker pattern
        UpdateWithChecker(2, 0);
        UpdateWithChecker(3, 1);

        alternatingFrames++; // Increment for next frame
    }
    else if (alternatingFrames == 3) {
        // Update without checker pattern
        UpdateWithChecker(3, 1);
        UpdateWithChecker(0, 0);

        alternatingFrames = 0; // Reset for next frame
    }

    std::swap(currentGrid, nextGrid); // Swap the grids to apply updates



}

void Simulation::updateCell(int x, int y) {
    switch (getCellType(read(x, y))) {
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

    default:
        const Cell& c = grid[y][x];
        std::cerr << "updateCell - Unknown type (int): " << static_cast<int>(c.type)
            << ", string: " << cellTypeToString(getCellType(c))
            << ", category: " << static_cast<int>(c.category) << "\n";
    }
}

void Simulation::updateSand(int x, int y) {

    Cell& cell = read(x, y);

    
    if (cell.lastUpdate >= 300) {
        if (frameCounter % 90 != 0) {
            cell.lastUpdate++;
            (*nextGrid)[y][x] = cell; // Write the current cell to the next grid
            return;
        }
    }
    else if (cell.lastUpdate >= 60) {
        if (frameCounter % 30 != 0) {
            cell.lastUpdate++;
			(*nextGrid)[y][x] = cell; // Write the current cell to the next grid
            return;
        }
    }

    
    if (cell.temperature >= 1700) (*nextGrid)[y][x].type = toRawType(CellType::GLASS);

    fall(x, y);
}

void Simulation::updateWater(int x, int y) {

    Cell& cell = read(x, y);


    if (cell.lastUpdate >= 60 && frameCounter % 10 != 0) {
		cell.lastUpdate++;
		(*nextGrid)[y][x] = cell; // Write the current cell to the next grid
        return; // Update less if the cell has not been updated recently
    }

    if (cell.temperature >= 100) {
        // Convert water to steam if it reaches boiling point
        (*nextGrid)[y][x].type = toRawType(CellType::STEAM);
    }


    flow(x, y);
}

void Simulation::updateFire(int x, int y) {
    constexpr int MAX_FIRE_HEIGHT = 6; // Maximum height of fire

    Cell& cell = read(x, y);


    int maxLife = cell.data.other.parentCell ? 90 : 40;
    if (cell.data.other.lifetime >= maxLife + (rand() % 5)) {
        clearCells(x, y, cell.temperature); // Clear the cell if it has lived too long
        return;
    }

    int r = rand();
    int maxHeight = std::max(1, MAX_FIRE_HEIGHT - (cell.data.other.lifetime / 10));

    // Only allow spreading if random roll + age is below a threshold
    if ((r % 100) > 80 || cell.data.other.lifetime > 40) return; // Old fire spreads less

    // Try only 1-2 cells upward, not the full MAX_FIRE_HEIGHT
    int maxSpreadHeight = 1 + (r % 2); // Try to go 1 or 2 cells up

    float spreadChance = 1.0f - (float)cell.data.other.lifetime / 50.0f;
    if ((rand() % 100) > (spreadChance * 100)) return; // Spreads less over time

    for (int i = 1; i <= maxSpreadHeight; ++i) {

        int fireNeighbors = 0;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;
                if (inBounds(nx, ny) && read(nx, ny).type == toRawType(CellType::FIRE)) fireNeighbors++;
            }
        }

        if (fireNeighbors >= 6) clearCells(x, y); return; // Too much fire nearby, skip spreading


        int dx = (rand() % 3) - 1; // -1, 0, or 1
        int nx = x + dx;
        int ny = y - i;

        if (!inBounds(nx, ny)) continue;
        Cell& targetCell = read(nx, ny);

        if (targetCell.type == toRawType(CellType::EMPTY)) {
            // Slightly vary new cell lifetime to avoid synchronized death
            int newLifetime = cell.data.other.lifetime + 1 + (rand() % 3);

            write(nx, ny, createCell(
                CellType::FIRE,
                0,
                cellProperties[PropertyIndexMap[CellType::FIRE]].temperature,
                false
            ));
            (*nextGrid)[ny][nx].data.other.lifetime = newLifetime;
            break; // Only spread to one cell this frame
        }
    }

    write(x, y, cell); // Write the current cell to the next grid
}

void Simulation::updateOil(int x, int y) {

    Cell& cell = read(x, y);


}

void Simulation::updateSteam(int x, int y) {

    Cell& cell = read(x, y);


    if (cell.temperature <= 100) (*nextGrid)[y][x].type = toRawType(CellType::WATER); // Convert steam back to water if it cools down

    rise(x, y); // Steam rises by default
}

void Simulation::updateSmoke(int x, int y) {

    Cell& cell = read(x, y);


    if (frameCounter > (600 + rand() % 100)) { // Smoke dissipates after a while
        (*nextGrid)[y][x].type = toRawType(CellType::EMPTY);
        return;
    }

    rise(x, y);
}

void Simulation::updateElectricity(int x, int y) {

    Cell& cell = read(x, y);

	cell.data.other.lifetime++; // Increment lifetime
    if (cell.data.other.lifetime >= 8) {
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
            if (inBounds(nx, ny) && read(nx, ny).type == toRawType(CellType::ELECTRICITY)) {
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
    float spreadChance = 1.0f - (float)cell.data.other.lifetime / 8.0f;
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
        if (inBounds(nx, ny) && read(nx, ny).type == toRawType(CellType::EMPTY)) {
            write(nx, ny, createCell(CellType::ELECTRICITY, 0.0f, cell.temperature, false));
            (*nextGrid)[ny][nx].data.other.lifetime = cell.data.other.lifetime + 1;
        }
    }

    // Write the current cell back to the next grid
    write(x, y, cell);

}

void Simulation::updateGlass(int x, int y) {

    Cell& cell = read(x, y);

    fall(x, y);

}

void Simulation::updateStone(int x, int y) {

    Cell& cell = read(x, y);

    if (cell.temperature >= 1205) (*nextGrid)[y][x].type = toRawType(CellType::LAVA);

    fall(x, y);

}

void Simulation::updateWood(int x, int y) {

    Cell& cell = read(x, y);



    if (cell.temperature >= 300) {
		// Wood ignites
		(*nextGrid)[y][x].type = toRawType(CellType::FIRE);
    }


    fall(x, y);

    
}

void Simulation::updateLava(int x, int y) {

    Cell& cell = read(x, y);

    if (cell.temperature <= 1195) (*nextGrid)[y][x].type = toRawType(CellType::STONE);

    flow(x, y);

}

void Simulation::updateCold(int x, int y) {

    Cell& cell = read(x, y);

    clearCells(x, y, cell.temperature); // Clear the cell but retain temperature
    // Cold cells do not move or interact, just make thing really cold for some time

}

sf::Color Simulation::getDebugColor(const Cell& cell) {

    if (debugModeIndex == 1) {
        return getTemperatureColor(cell.temperature);
    }
	if (debugModeIndex == 2) {
        return getLowTemperatureColor(cell.temperature);
	}
    if (debugModeIndex == 3) {
        return getLifetimeColor(getLifetime(cell));
    }

    if (debugModeIndex == 0) {
        // Default view, return color based on cell type
        return getColor(getCellType(cell));
    }

    assert(debugModeIndex >= 0 && debugModeIndex <= 9); // Ensure debugModeIndex is within bounds

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

    float t = std::clamp(static_cast<float>(lifetime / 1000), 0.0f, 1.0f);

    if (t < 0.33f) {
        // Black → Blue
        float localT = t / 0.33f;
        return lerpColor(sf::Color(0, 0, 0), sf::Color(0, 0, 255), localT);
    }
    else if (t < 0.66f) {
        // Blue → Orange
        float localT = (t - 0.33f) / (0.33f);
        return lerpColor(sf::Color(0, 0, 255), sf::Color(255, 165, 0), localT);
    }
    else {
        // Orange → Red
        float localT = (t - 0.66f) / (0.33f);
        return lerpColor(sf::Color(255, 165, 0), sf::Color(255, 0, 0), localT);
    }
}

sf::Color Simulation::getLowTemperatureColor(float temp) {
    // Clamp for safety
    temp = std::clamp(temp, -50.0f, 100.0f);

    // Output color
    sf::Color color;

    if (temp <= -25.0f) {
        float t = (temp + 50.0f) / 25.0f; // -50 to -25
        color = lerpColor(sf::Color(100, 0, 150, 50), sf::Color(0, 0, 180, 80), t);
    }
    else if (temp <= 0.0f) {
        float t = (temp + 25.0f) / 25.0f; // -25 to 0
        color = lerpColor(sf::Color(0, 0, 180, 80), sf::Color(0, 32, 64, 40), t);
    }
    else if (temp <= 35.0f) {
        float t = temp / 35.0f;
        // More rapid visual transition + alpha rising
        color = lerpColor(sf::Color(0, 32, 64, 40), sf::Color(255, 200, 50, 180), std::sqrt(t));
    }
    else {
        float t = (temp - 35.0f) / (100.0f - 35.0f);
        color = lerpColor(sf::Color(255, 200, 50, 180), sf::Color(255, 0, 0, 255), t);
    }

    return color;
}

sf::Color Simulation::getTemperatureColor(float temp) {

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

void Simulation::drawDebugText(sf::RenderWindow& window) {
    auto mouse = sf::Mouse::getPosition(window);
    int x = mouse.x / PIXEL_SIZE;
    int y = mouse.y / PIXEL_SIZE;

    if (!inBounds(x, y)) return;

	Cell& cell = read(x, y);


    std::ostringstream oss;
    oss << std::fixed << std::showpoint << std::setprecision(2);

    oss << "Position: (" << x << ", " << y << ")\n"
        << "Type: " << cellTypeToString(getCellType(cell)) << "\n"
        << "Density: " << getDensity(cell) << "\n"
        << "Temperature: " << cell.temperature << "\n"
        << "Lifetime: " << getLifetime(cell) << "\n"
        << "Can Fall: " << (getCanFall(cell) ? "Yes" : "No") << "\n"
        << "Last update: " << cell.lastUpdate << "\n"
        << "Is parent: " << (isParentCell(cell) ? "Yes" : "No") << "\n";

    std::string debugInfo = oss.str();


	sf::Font font;
	if (!font.loadFromFile("arial.ttf")) {
		assert(false && "Error loading font");
		return;
	}
	sf::Text text(debugInfo, font, 20);
	text.setFillColor(sf::Color::Magenta);
	text.setPosition(10.f, 10.f);
	window.draw(text);
}

void Simulation::draw(sf::RenderWindow& window) {
    sf::VertexArray cells(sf::Quads);

    

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            const Cell& cell = read(x, y);


            float px = x * PIXEL_SIZE;
            float py = y * PIXEL_SIZE;


            sf::Color color;

            if (debugModeIndex != 0) {
                color = getDebugColor(cell);
            }
            else {
                color = getColor(static_cast<CellType>(cell.type));
            }


            // Add the cell as a quad
            cells.append(sf::Vertex(sf::Vector2f(px, py), color));
            cells.append(sf::Vertex(sf::Vector2f(px + PIXEL_SIZE, py), color));
            cells.append(sf::Vertex(sf::Vector2f(px + PIXEL_SIZE, py + PIXEL_SIZE), color));
            cells.append(sf::Vertex(sf::Vector2f(px, py + PIXEL_SIZE), color));
        }
    }
    
    window.draw(cells);
    
    if (debugText) {
        drawDebugText(window); // Draw debug text if enabled
    }
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

	if (!File.is_open()) {
		std::cerr << "Error opening log file!\n";
		return 1;
	}
	bool logFPS = false; // Enable FPS logging to file

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
                sim->update(); // Updates only when not paused
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

       

		// get the current time after rendering
        auto end = std::chrono::high_resolution_clock::now();

        // Calculate and display FPS every 0.5 seconds
        if (frameCounter % static_cast<int>(lastFPS / 2) == 0) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - current).count();
            lastFPS = 1000000.f / duration;
            std::cout << "FPS: " << static_cast<int>(lastFPS) << '\n';
            frameCounter = 0; // Reset frame counter


			// Log FPS to file if enabled
            if (logFPS) {
				File << "FPS: " << lastFPS << '\n'; // Log FPS to file
            }
        }

        frameCounter++;
    }


    return 0;
}


