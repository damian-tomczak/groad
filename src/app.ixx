#include "utils.hpp"

export module App;

import utils;

namespace spera
{
export class App final
{
    NOT_COPYABLE_AND_MOVEABLE(App);

  public:
    App() = default;
};
} // namespace spera