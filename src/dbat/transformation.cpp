#include "dbat/transformation.h"

namespace trans {

    double Transformation::getBonus(struct char_data *ch, int location, int specific) {
        return 0.0;
    }

    std::optional<int> Transformation::overrideAppearance(struct char_data *ch, int mode) {
        return std::nullopt;
    }

    void Transformation::onUpdate(double deltaTime) {

    }

    std::string Transformation::descriptionLine(struct char_data *ch) {
        return "";
    }

    void Transformation::onLoseBodyPart(struct char_data *ch, int limb) {

    }

    void Transformation::onTransform(struct char_data *ch, bool voluntary) {

    }

    void Transformation::onRevert(struct char_data *ch, bool voluntary) {

    }

}