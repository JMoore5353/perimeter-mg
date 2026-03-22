#ifndef PERIMETER_PERIMETER_ENVIRONMENT_ACTION_H
#define PERIMETER_PERIMETER_ENVIRONMENT_ACTION_H

namespace perimeter::environment {

enum class Action {
    EAST,
    NORTHEAST,
    NORTHWEST,
    WEST,
    SOUTHWEST,
    SOUTHEAST,
    STAY,
};

}  // namespace perimeter::environment

#endif  // PERIMETER_PERIMETER_ENVIRONMENT_ACTION_H
