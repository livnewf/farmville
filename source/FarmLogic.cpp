#include "FarmLogic.h"
#include "displayobject.hpp"
#include <unistd.h>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <array>

// for Locations
struct Location
{
    int x;
    int y;
    Location(int x_, int y_) : x(x_), y(y_) {}
    Location() : x(0), y(0) {}
};

struct Offset
{
    int id;
    Location home_position;
    Location approach_offset;
};

std::array<Offset, 3> chicken_offset = {{// chicken1 approaches from left
                                         {0, {50, 400}, {-20, 0}},
                                         // chicken2 approaches from below
                                         {1, {100, 400}, {0, -75}},
                                         // chicken3 approaches from above
                                         {2, {150, 400}, {0, 75}}}};

// paths for each nest
struct Path
{
    std::vector<Location> points;
};
Path farmer_barn_path = {std::vector<Location>{
        {20,375}, {20,100}, {20,50}
    }};

// create 3 lanes per nest (one for each chicken)
std::array<Path, 4> nest_paths;

void init_paths()
{
    //chicken 1 path to nest 1 and 2
    nest_paths[0].points = {
        {100, 575},
        {700, 575},
        {400, 575}};
    //chicken 2 path to nest 1 and 2
    nest_paths[1].points = {
        {130, 530},
        {670, 530},
        {370, 530}};
    //chicken 3 path to all nests
    nest_paths[2].points = {
        {130, 430},
        {670, 430},
        {370, 430}};
    //farmer path to all nests
    nest_paths[3].points = {
        {80, 375},
        {720, 375},
        {380, 375}};
};

// for farmers and chickens
struct Egg
{
    int egg_id;
    int nest_id;
    std::shared_ptr<DisplayObject> sprite;
    Egg(int id_, int nest_id_, std::shared_ptr<DisplayObject> sprite_)
        : egg_id(id_), nest_id(nest_id_), sprite(sprite_) {}
};

struct Nest
{
    std::mutex nest;
    std::atomic<int> eggs = 0;
    std::deque<Egg> buffer;
    static const int LEN = 3;
    std::condition_variable cv_chicken;
    std::condition_variable cv_farmer;
    bool occupied = false;
};
Nest nests[3];

// store barn stats
struct Storage
{
    std::atomic<int> item1{0};
    std::atomic<int> item2{0};
    std::string item1_name;
    std::string item2_name;
    std::condition_variable cv;
    std::mutex mtx;
    Location location = Location(0, 0);
    Location bakery_loc = Location(0,0);

    Storage(std::string name1, std::string name2, int x, int y, int x2, int y2)
    {
        item1_name = name1;
        item2_name = name2;
        location = Location(x, y);
        bakery_loc = Location(x2, y2);
    }
};
Storage b1 = Storage("eggs", "butter", 100, 50, -1, -1);
Storage b2 = Storage("sugar", "flour", 100, 150, -1, -1);
Storage t1 = Storage("eggs", "butter", -1, -1, 500, 50);
Storage t2 = Storage("sugar", "flour", -1, -1, 500, 150);

// for cake
struct Cake
{
    int id;
    std::shared_ptr<DisplayObject> sprite;
    Cake(int id_, std::shared_ptr<DisplayObject> sprite_)
        : id(id_), sprite(sprite_) {}
};

// for bakery (production and sales)
struct BakeryStorage
{
    // to store ingredients and ensure do not exceed capacity
    std::atomic<int> eggs = 0;
    std::atomic<int> butter = 0;
    std::atomic<int> sugar = 0;
    std::atomic<int> flour = 0;
    std::atomic<int> cakes = 0;
    std::condition_variable cake_cv;
    std::mutex child;
    std::mutex cake;
    std::deque<Cake> buffer;
    DisplayObject bakeryeggs[6] = {
        DisplayObject("egg", 10, 20, 1, 14),
        DisplayObject("egg", 10, 20, 1, 15),
        DisplayObject("egg", 10, 20, 1, 16),
        DisplayObject("egg", 20, 20, 1, 46),
        DisplayObject("egg", 20, 20, 1, 47),
        DisplayObject("egg", 20, 20, 1, 48)};
    DisplayObject bakeryflour[6] = {
        DisplayObject("flour", 20, 20, 1, 17),
        DisplayObject("flour", 20, 20, 1, 18),
        DisplayObject("flour", 20, 20, 1, 19),
        DisplayObject("flour", 20, 20, 1, 49),
        DisplayObject("flour", 20, 20, 1, 50),
        DisplayObject("flour", 20, 20, 1, 51)};
    DisplayObject bakerybutter[6] = {
        DisplayObject("butter", 20, 20, 1, 20),
        DisplayObject("butter", 20, 20, 1, 21),
        DisplayObject("butter", 20, 20, 1, 22),
        DisplayObject("butter", 20, 20, 1, 52),
        DisplayObject("butter", 20, 20, 1, 53),
        DisplayObject("butter", 20, 20, 1, 54)};
    DisplayObject bakerysugar[6] = {
        DisplayObject("sugar", 20, 20, 1, 23),
        DisplayObject("sugar", 20, 20, 1, 24),
        DisplayObject("sugar", 20, 20, 1, 25),
        DisplayObject("sugar", 20, 20, 1, 55),
        DisplayObject("sugar", 20, 20, 1, 56),
        DisplayObject("sugar", 20, 20, 1, 57)};
    DisplayObject bakerycake[6] = {
        DisplayObject("cake", 20, 20, 1, 26),
        DisplayObject("cake", 20, 20, 1, 27),
        DisplayObject("cake", 20, 20, 1, 28),
        DisplayObject("cake", 20, 20, 1, 43),
        DisplayObject("cake", 20, 20, 1, 44),
        DisplayObject("cake", 20, 20, 1, 45)};

    static const int LEN = 6;

    void update_item(std::string name, int amount)
    {
        if (name == "eggs")
        {
            eggs += amount;
            bakeryeggs[eggs - 1].updateFarm();
        }
        else if (name == "butter")
        {
            butter += amount;
            bakerybutter[butter - 1].updateFarm();
        }
        else if (name == "flour")
        {
            flour += amount;
            bakeryflour[flour - 1].updateFarm();
        }
        else
        {
            sugar += amount;
            bakerysugar[sugar - 1].updateFarm();
        }
    }

    int check_inventory(std::string name)
    {
        if (name == "eggs")
        {
            return eggs;
        }
        else if (name == "butter")
        {
            return butter;
        }
        else if (name == "flour")
        {
            return flour;
        }
        else
        {
            return sugar;
        }
    };

    void set_pos()
    {
        bakeryeggs[0].setPos(510, 130);
        bakeryeggs[1].setPos(520, 130);
        bakeryeggs[2].setPos(530, 130);
        bakeryeggs[3].setPos(500, 130);
        bakeryeggs[4].setPos(540, 130);
        bakeryeggs[5].setPos(490, 130);

        bakeryflour[0].setPos(500, 110);
        bakeryflour[1].setPos(520, 110);
        bakeryflour[2].setPos(540, 110);
        bakeryflour[3].setPos(480, 110);
        bakeryflour[4].setPos(560, 110);
        bakeryflour[5].setPos(460, 110);

        bakerysugar[0].setPos(500, 90);
        bakerysugar[1].setPos(520, 90);
        bakerysugar[2].setPos(540, 90);
        bakerysugar[3].setPos(560, 90);
        bakerysugar[4].setPos(480, 90);
        bakerysugar[5].setPos(460, 90);

        bakerybutter[0].setPos(500, 70);
        bakerybutter[1].setPos(520, 70);
        bakerybutter[2].setPos(540, 70);
        bakerybutter[3].setPos(560, 70);
        bakerybutter[4].setPos(480, 70);
        bakerybutter[5].setPos(460, 70);

        bakerycake[0].setPos(600, 200);
        bakerycake[1].setPos(620, 200);
        bakerycake[2].setPos(640, 200);
        bakerycake[3].setPos(600, 180);
        bakerycake[4].setPos(620, 180);
        bakerycake[5].setPos(640, 180);
    };
};

BakeryStorage bs = BakeryStorage();

void FarmLogic::run()
{
    bs.set_pos();
    init_paths();

    BakeryStats stats;

    std::srand(std::time(0));
    DisplayObject chicken1("chicken", 40, 40, 2, 0);
    DisplayObject chicken2("chicken", 40, 40, 2, 1);
    DisplayObject chicken3("chicken", 40, 40, 2, 2);
    DisplayObject nest1("nest", 60, 40, 0, 39);
    DisplayObject nest2("nest", 60, 40, 0, 3);
    DisplayObject nest3("nest", 60, 40, 0, 29);
    DisplayObject nest1eggs[3] = {
        DisplayObject("egg", 10, 20, 1, 4),
        DisplayObject("egg", 10, 20, 1, 5),
        DisplayObject("egg", 10, 20, 1, 6)};
    DisplayObject nest2eggs[3] = {
        DisplayObject("egg", 10, 20, 1, 35),
        DisplayObject("egg", 10, 20, 1, 36),
        DisplayObject("egg", 10, 20, 1, 37)};
    DisplayObject nest3eggs[3] = {
        DisplayObject("egg", 10, 20, 1, 40),
        DisplayObject("egg", 10, 20, 1, 41),
        DisplayObject("egg", 10, 20, 1, 42)};
    DisplayObject cow1("cow", 60, 60, 2, 7);
    DisplayObject cow2("cow", 60, 60, 2, 30);
    DisplayObject truck1("truck", 80, 60, 2, 8);
    DisplayObject truck2("truck", 80, 60, 2, 38);
    DisplayObject farmer("farmer", 30, 60, 2, 9);
    DisplayObject child1("child", 30, 60, 2, 10);
    DisplayObject child2("child", 30, 60, 2, 31);
    DisplayObject child3("child", 30, 60, 2, 32);
    DisplayObject child4("child", 30, 60, 2, 33);
    DisplayObject child5("child", 30, 60, 2, 34);
    DisplayObject children[5] = {child1, child2, child3, child4, child5};
    DisplayObject barn1("barn", 100, 100, 0, 11);
    DisplayObject barn2("barn", 100, 100, 0, 12);
    DisplayObject bakery("bakery", 250, 250, 0, 13);

    std::array<DisplayObject *, 3> display_nests = {nest1eggs, nest2eggs, nest3eggs};

    Location bakeryline[5] = {
        Location(620, 100),
        Location(650, 100),
        Location(680, 100),
        Location(710, 100),
        Location(740, 100)};

    Location nest_locations[3] = {
        Location(100, 500),
        Location(700, 500),
        Location(400, 450)};

    Location child_bakery_location = Location(550, 150);

    Location intersection = Location(400, 100);

    Location farmer_initial = Location(50, 350);
    Location farmer_location = Location(20, 50);

    chicken1.setPos(300, 600);
    chicken2.setPos(300, 550);
    chicken3.setPos(300, 400);
    nest1.setPos(100, 500);
    nest2.setPos(700, 500);
    nest3.setPos(400, 450);
    nest1eggs[0].setPos(90, 507);
    nest1eggs[1].setPos(100, 507);
    nest1eggs[2].setPos(110, 507);
    nest2eggs[0].setPos(690, 507);
    nest2eggs[1].setPos(700, 507);
    nest2eggs[2].setPos(710, 507);
    nest3eggs[0].setPos(390, 457);
    nest3eggs[1].setPos(400, 457);
    nest3eggs[2].setPos(410, 457);
    cow1.setPos(730, 190);
    cow2.setPos(740, 240);
    truck1.setPos(100, 50);
    truck2.setPos(100, 150);
    farmer.setPos(50, 350);
    child1.setPos(620, 100);
    child2.setPos(650, 100);
    child3.setPos(680, 100);
    child4.setPos(710, 100);
    child5.setPos(740, 100);
    barn1.setPos(50, 50);
    barn2.setPos(50, 150);
    bakery.setPos(550, 150);

    chicken1.updateFarm();
    chicken2.updateFarm();
    chicken3.updateFarm();
    nest1.updateFarm();
    nest2.updateFarm();
    nest3.updateFarm();
    cow1.updateFarm();
    cow2.updateFarm();
    truck1.updateFarm();
    truck2.updateFarm();
    farmer.updateFarm();
    child1.updateFarm();
    child2.updateFarm();
    child3.updateFarm();
    child4.updateFarm();
    child5.updateFarm();
    barn1.updateFarm();
    barn2.updateFarm();
    bakery.updateFarm();

    std::mutex intersection_mtx;
    std::condition_variable intersection_cv;
    bool intersection_occupied = false;

    static auto within_radius = [&](int x, int y, Location l) -> bool
    {
        if (l.x - 160 <= x && x <= l.x + 160)
        {
            if (l.y - 120 <= y && y <= l.y + 120)
            {
                return true;
            }
        }
        return false;
    };

    auto move_lock = [&](DisplayObject &obj, Location &loc) -> void
    {
        double dx = loc.x - obj.x;
        double dy = loc.y - obj.y;
        double dist = std::hypot(dx, dy);
        double step = 3.0;

        if (dist == 0)
            return;

        double step_x = dx / dist * step;
        double step_y = dy / dist * step;
        double curr_x = obj.x;
        double curr_y = obj.y;
        bool in_intersection = false;

        while (std::hypot(loc.x - curr_x, loc.y - curr_y) > step)
        {
            double next_x = curr_x + step_x;
            double next_y = curr_y + step_y;

            if (obj.texture == "truck")
            {
                bool about_to_enter = within_radius(next_x, next_y, intersection);
                if (about_to_enter && !in_intersection)
                {
                    {
                        std::unique_lock<std::mutex> lock(intersection_mtx);
                        std::cout << obj.texture << " " << obj.id << "lock at " << next_x << ", " << next_y << std::endl;
                        intersection_cv.wait(lock, [&]()
                                             { return !intersection_occupied; });
                        intersection_occupied = true;
                        in_intersection = true;

                        while (within_radius(next_x, next_y, intersection) &&
                               std::hypot(loc.x - next_x, loc.y - next_y) > step)
                        {
                            curr_x = next_x;
                            curr_y = next_y;
                            obj.setPos(curr_x, curr_y);
                            obj.updateFarm();

                            next_x += step_x;
                            next_y += step_y;
                            std::this_thread::sleep_for(std::chrono::milliseconds(16));
                        }
                        curr_x = next_x;
                        curr_y = next_y;
                        // leaving intersection, release intersection and set to false
                        intersection_occupied = false;
                        in_intersection = false;
                    }
                    // intersection lock is now free
                    // wake up waiting truck
                    std::cout << "released intersection lock" << std::endl;
                    intersection_cv.notify_one();
                    
                }
                else if (!in_intersection)
                {
                    // normal movement outside intersection
                    curr_x = next_x;
                    curr_y = next_y;
                    obj.setPos(curr_x, curr_y);
                    obj.updateFarm();
                }
            }

            else
            {
                // non-truck objects
                curr_x = next_x;
                curr_y = next_y;
                obj.setPos(curr_x, curr_y);
                obj.updateFarm();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        obj.setPos(loc.x, loc.y);
        obj.updateFarm();
    };

    auto move_to = [&](DisplayObject &obj, Location &loc) -> void
    {
        move_lock(obj, loc);
    };

    auto move_along_path = [&](DisplayObject &obj, const std::vector<Location> &points, int start, int nest) -> void
    {
        // take necessary steps to get to desired nest
        for (int i = start; i < nest + 1; i++)
        {
            const auto &point = points[i];
            // move to each point on path in sequence
            double dx = point.x - obj.x;
            double dy = point.y - obj.y;
            double dist = std::hypot(dx, dy);
            double step = 3.0;

            if (dist == 0)
                continue;

            double step_x = dx / dist * step;
            double step_y = dy / dist * step;
            double curr_x = obj.x;
            double curr_y = obj.y;

            while (std::hypot(point.x - curr_x, point.y - curr_y) > step)
            {
                curr_x += step_x;
                curr_y += step_y;

                obj.setPos(curr_x, curr_y);
                obj.updateFarm();

                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }

            // final step
            obj.setPos(point.x, point.y);
            obj.updateFarm();
        }
    };

    static auto random_idx = [](int capacity) -> int
    {
        return std::rand() % capacity;
    };

    auto chicken_nest_loop = [&](DisplayObject &chicken) -> void
    {
        //Offset offset = chicken_offset[chicken.id];

        while (true)
        {
            int nest_num = 0;
            //chicken1 cannot go to nest3
            if(chicken.id == 0) {nest_num = 2;}
            //chickens 3 and 2 can go anywhere
            else {nest_num = 3;}
            int idx = random_idx(nest_num);
            Nest &nest = nests[idx];
            Path &path =  nest_paths[chicken.id];//nest_paths[idx][chicken.id];

            move_along_path(chicken, path.points, 0, idx);
            {
                std::unique_lock<std::mutex> nest_lock(nest.nest);
                nest.cv_chicken.wait_for(nest_lock, std::chrono::milliseconds(100), [&]()
                                     { return !nest.occupied && nest.eggs < 3; });

                nest.occupied = true;
                //move_along_path(chicken, path.points, 1, 2);
                move_to(chicken, nest_locations[idx]);
                // acquired the nest and updated
                // create a display sprite for the egg
                DisplayObject *nest_eggs = display_nests[idx];
                if (nest.eggs < 3)
                {
                    int egg_index = nest.eggs;
                    nest.eggs += 1;
                    auto egg_sprite = std::make_shared<DisplayObject>(nest_eggs[egg_index]);
                    // create and lay the logical egg with egg_id, nest_id, display egg
                    Egg egg(idx, idx, egg_sprite);
                    nest.buffer.push_back(egg);
                    egg.sprite->updateFarm();
                    stats.eggs_laid += 1;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                // step away to point just before the nest on the path
                Location next_location = path.points[idx];
                move_to(chicken, next_location);
                nest.occupied = false;
            }
            nest.cv_farmer.notify_one();
            nest.cv_chicken.notify_one();
            chicken.updateFarm();
            // release visual nest lock and reset thread
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    };

    // farmer loop - always
    auto farmer_loop = [&]() -> void
    {
        while (true)
        {
            int idx = random_idx(3);
            Nest &nest = nests[idx];
            DisplayObject *nest_eggs = display_nests[idx];
            Path &path = nest_paths[3];

            // step to nest
            move_along_path(farmer, path.points, 0, idx);

            int egg_count = 0;
            // try to go to the nest
            {
                std::unique_lock<std::mutex> nest_lock(nest.nest);
                nest.cv_farmer.wait_for(nest_lock, std::chrono::milliseconds(50), [&]()
                                    { return !nest.occupied && nest.eggs > 0; });

                // reserve nest for collection
                nest.occupied = true;
                // nest is acquired, move
                //move_along_path(farmer, path.points, 1, 2);
                move_to(farmer,nest_locations[idx]);
                egg_count += nest.eggs;
                // eggs to pick up, bring them to the barn
                if (egg_count > 0)
                {
                    for (int i = 0; i < nest.eggs; i++)
                    {
                        nest_eggs[i].erase();
                    }
                    nest.buffer.clear();
                    nest.eggs = 0;

                    // and move to barn
                    Location next_location = path.points[idx];
                    move_to(farmer, next_location);
                    
                    nest.occupied = false;
                }
            }

            nest.cv_chicken.notify_all();
            move_along_path(farmer, farmer_barn_path.points, 0, 2);
            b1.item1 += egg_count;

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    };
    // random crop production
    auto random_crop_production = [&]() -> void
    {
        while (true)
        {
            int count = random_idx(3);
            int butter = count % 3 + 1;
            int flour = count % 5 + 2;
            int sugar = count % 7 + 3;

            stats.butter_produced += butter;
            stats.flour_produced += flour;
            stats.sugar_produced += sugar;

            b1.item2 += butter;
            b2.item1 += sugar;
            b2.item2 += flour;

            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
    };

    // fix
    auto unload_items = [&](Storage &t) -> void
    {
        // truck still has items and the items, if added, do not exceed capacity
        std::array<std::string, 2> names = {t.item1_name, t.item2_name};
        while (t.item1 != 0 && t.item2 != 0 && bs.check_inventory(names[0]) + 1 < 6 && bs.check_inventory(names[1]) + 1 < 6)
        {
            {
                // truck unloads 1 of each item at a time
                // bakery adds these items to inventory
                t.item1 -= 1;
                t.item2 -= 1;
                bs.update_item(names[0], 1);
                bs.update_item(names[1], 1);
            }
        }
    };

    // truck loop - conditional - fix
    auto truck_loop = [&](DisplayObject &truck, Storage &t, Storage &b) -> void
    {
        while (true)
        {
            // truck go to barn
            move_to(truck, b.location);

            // barn has at least 3 of each item
            if (b.item1.load() >= 3 && b.item2.load() >= 3)
            {
                // now it's safe to pick up all items
                b.item1 -= 3;
                b.item2 -= 3;
                t.item1 += 3;
                t.item2 += 3;

                // truck leaves with full load (3 of each item) CS
                // drive to bakery for delivery
                move_to(truck, intersection);
                move_to(truck, t.bakery_loc);

                // at the bakery
                unload_items(t);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    };

    // oven loop
    auto bakery_loop = [&]() -> void
    {
        while (true)
        {
            // check if cake can be made
            bool made_cake = false;
            bool can_make = true;
            can_make = can_make && (bs.eggs >= 2);
            can_make = can_make && (bs.flour >= 2);
            can_make = can_make && (bs.butter >= 2);
            can_make = can_make && (bs.sugar >= 2);
            can_make = can_make && (bs.cakes < 3);
            if (can_make)
            {
                {
                    // claim stock shelf, to block cake from child purchase
                    std::unique_lock<std::mutex> cake_lock(bs.cake);
                    bs.cake_cv.wait(cake_lock, [&]()
                                    { return bs.eggs >= 2 &&
                                             bs.flour >= 2 &&
                                             bs.butter >= 2 &&
                                             bs.sugar >= 2 &&
                                             bs.cakes < 3; });
                    // remove the ingredients from the stock
                    bs.eggs -= 1;
                    bs.bakeryeggs[bs.eggs].erase();
                    bs.eggs -= 1;
                    bs.bakeryeggs[bs.eggs].erase();
                    bs.sugar -= 1;
                    bs.bakerysugar[bs.sugar].erase();
                    bs.sugar -= 1;
                    bs.bakerysugar[bs.sugar].erase();
                    bs.flour -= 1;
                    bs.bakeryflour[bs.flour].erase();
                    bs.flour -= 1;
                    bs.bakeryflour[bs.flour].erase();
                    bs.butter -= 1;
                    bs.bakerybutter[bs.butter].erase();
                    bs.butter -= 1;
                    bs.bakerybutter[bs.butter].erase();

                    bs.cakes+=3;
                    auto cake_sprite1 = std::make_shared<DisplayObject>(bs.bakerycake[bs.cakes - 3]);
                    auto cake_sprite2 = std::make_shared<DisplayObject>(bs.bakerycake[bs.cakes - 2]);
                    auto cake_sprite3 = std::make_shared<DisplayObject>(bs.bakerycake[bs.cakes - 1]);
                    // bake the logical cake and render it to the 
                    Cake cake1(bs.cakes - 3, cake_sprite1);
                    Cake cake2(bs.cakes - 2, cake_sprite2);
                    Cake cake3(bs.cakes - 1, cake_sprite3);
                    bs.buffer.push_back(cake1);
                    bs.buffer.push_back(cake2);
                    bs.buffer.push_back(cake3);

                    cake_sprite1->updateFarm();
                    cake_sprite2->updateFarm();
                    cake_sprite3->updateFarm();
                    // move cake to stock shelf for purchase
                    stats.cakes_produced += 3;
                    stats.butter_used += 2;
                    stats.sugar_used += 2;
                    stats.eggs_used += 2;
                    stats.flour_used += 2;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                bs.cake_cv.notify_all();
            }
            // update stats (variables are all atomic)

            //  adjust stats on the consumer side, increment by 2 for each
            //  remove the cake from oven (release lock out of scope)
            //  END Cs
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    };

    auto children_loop = [&](DisplayObject &child, int child_idx) -> void
    {
        Path child_path = {std::vector<Location>{
            //move up
            {bakeryline[child_idx].x, bakeryline[child_idx].y + 30},
            // move across
            {child_bakery_location.x, bakeryline[child_idx].y + 30},
            // move to bakery
            {child_bakery_location.x, child_bakery_location.y},
            // move down
            {child_bakery_location.x, child_bakery_location.y - 100},
            // move across
            {bakeryline[child_idx].x, child_bakery_location.y - 100},
            // move up
            {bakeryline[child_idx].x, bakeryline[child_idx].y}}
        };
        while (true)
        {
            // try to go into the bakery
            {
                std::unique_lock<std::mutex> child_lock(bs.child);
                // bakery is occupied
                // lock on child entrance, all other children wait
                // go inside
                move_along_path(child, child_path.points, 0, 2);
                std::atomic<int> cakes_sold = 0;
                int k = random_idx(6) + 1;
                {
                    std::unique_lock<std::mutex> cake_lock(bs.cake);
                    while (cakes_sold < k)
                    {
                        bs.cake_cv.wait(cake_lock, [&]()
                                        { return bs.cakes > 0; });
                        bs.cakes -= 1;
                        bs.buffer.pop_front();
                        bs.bakerycake[bs.cakes].erase();
                        // child buys cake
                        cakes_sold += 1;
                        stats.cakes_sold += 1;
                    }
                    // child has bought all k cakes, walk away and release lock
                    move_to(child, child_path.points[3]);
                }
            }
            // leave bakery along path
            //move_to(child, down_location);
            move_along_path(child, child_path.points, 4, 5);
            // child walks away (back into line) and consume cake
            // releases bakery for next child
            child.updateFarm();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    };

    auto display_loop = [](BakeryStats &stats) -> void
    {
        while (true)
        {
            {
                DisplayObject::redisplay(stats); // safely read from theFarm
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };

    std::vector<std::thread> farm_threads;

    // display thread
    farm_threads.emplace_back(display_loop, std::ref(stats));

    // create random crop thread
    farm_threads.emplace_back(random_crop_production);

    // create 3 chicken threads
    farm_threads.emplace_back(chicken_nest_loop, std::ref(chicken1));
    farm_threads.emplace_back(chicken_nest_loop, std::ref(chicken2));
    farm_threads.emplace_back(chicken_nest_loop, std::ref(chicken3));

    // create farmer thread
    farm_threads.emplace_back(farmer_loop);

    // create bakery thread
    farm_threads.emplace_back(bakery_loop);

    // create children threads
    farm_threads.emplace_back(children_loop, std::ref(child1), 1);
    farm_threads.emplace_back(children_loop, std::ref(child2), 2);
    farm_threads.emplace_back(children_loop, std::ref(child3), 3);
    farm_threads.emplace_back(children_loop, std::ref(child4), 4);
    farm_threads.emplace_back(children_loop, std::ref(child5), 5);

    // // create threads for truck objects
    farm_threads.emplace_back(truck_loop, std::ref(truck1), std::ref(t1), std::ref(b1));
    farm_threads.emplace_back(truck_loop, std::ref(truck2), std::ref(t2), std::ref(b2));

    for (auto &thread : farm_threads)
    {
        thread.join();
    }
}

void FarmLogic::start()
{
    std::thread([]()
                { FarmLogic::run(); })
        .detach();
}