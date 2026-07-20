#include <sol/sol.hpp>
#include <string>
#include <cstdio>

#include "vm/api/primitives.hpp"
#include "core/primitives/vector2.hpp"
#include "core/primitives/vector3.hpp"
#include "core/primitives/vector4.hpp"
#include "core/primitives/quaternion.hpp"
#include "core/primitives/matrix3.hpp"
#include "core/primitives/matrix4.hpp"
#include "core/primitives/transform.hpp"

using namespace core::ecs::primitives;

void primitives(lua_State* state) {
    sol::state_view view(state);

    view.new_usertype<Vector2>("Vector2",
        sol::constructors<Vector2(), Vector2(float, float)>(),
        "x", &Vector2::x,
        "y", &Vector2::y,
        "length", &Vector2::length,
        "squared", &Vector2::squared,
        "normalized", &Vector2::normalized,
        "absolute", &Vector2::absolute,
        "perpendicular", &Vector2::perpendicular,
        "dot", &Vector2::dot,
        "cross", &Vector2::cross,
        "reflect", &Vector2::reflect,
        "project", &Vector2::project,
        "clamped", &Vector2::clamped,
        "approximately", &Vector2::approximately,
        "minimum", &Vector2::minimum,
        "maximum", &Vector2::maximum,
        sol::meta_function::addition, [](const Vector2& alpha, const Vector2& beta) { return alpha + beta; },
        sol::meta_function::subtraction, [](const Vector2& alpha, const Vector2& beta) { return alpha - beta; },
        sol::meta_function::division, [](const Vector2& alpha, float scalar) { return alpha / scalar; },
        sol::meta_function::unary_minus, [](const Vector2& alpha) { return -alpha; },
        sol::meta_function::equal_to, [](const Vector2& alpha, const Vector2& beta) { return alpha == beta; },
        sol::meta_function::multiplication, sol::overload(
            [](const Vector2& alpha, float scalar) { return alpha * scalar; },
            [](float scalar, const Vector2& beta) { return beta * scalar; }
        ),
        sol::meta_function::to_string, [](const Vector2& alpha) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "Vector2(%f, %f)", static_cast<double>(alpha.x), static_cast<double>(alpha.y));
            return std::string(buffer);
        }
    );

    view.new_usertype<Vector3>("Vector3",
        sol::constructors<Vector3(), Vector3(float, float, float), Vector3(const Vector2&, float)>(),
        "x", &Vector3::x,
        "y", &Vector3::y,
        "z", &Vector3::z,
        "length", &Vector3::length,
        "squared", &Vector3::squared,
        "normalized", &Vector3::normalized,
        "absolute", &Vector3::absolute,
        "dot", &Vector3::dot,
        "cross", &Vector3::cross,
        "reflect", &Vector3::reflect,
        "project", &Vector3::project,
        "clamped", &Vector3::clamped,
        "interpolate", &Vector3::interpolate,
        "approximately", &Vector3::approximately,
        "minimum", &Vector3::minimum,
        "maximum", &Vector3::maximum,
        sol::meta_function::addition, [](const Vector3& alpha, const Vector3& beta) { return alpha + beta; },
        sol::meta_function::subtraction, [](const Vector3& alpha, const Vector3& beta) { return alpha - beta; },
        sol::meta_function::division, [](const Vector3& alpha, float scalar) { return alpha / scalar; },
        sol::meta_function::unary_minus, [](const Vector3& alpha) { return -alpha; },
        sol::meta_function::equal_to, [](const Vector3& alpha, const Vector3& beta) { return alpha == beta; },
        sol::meta_function::multiplication, sol::overload(
            [](const Vector3& alpha, const float scalar) { return alpha * scalar; },
            [](const float scalar, const Vector3& beta) { return beta * scalar; },
            [](const Vector3& alpha, const Vector3& beta) { return alpha * beta; }
        ),
        sol::meta_function::to_string, [](const Vector3& alpha) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "Vector3(%f, %f, %f)", static_cast<double>(alpha.x), static_cast<double>(alpha.y), static_cast<double>(alpha.z));
            return std::string(buffer);
        }
    );

    view.new_usertype<Vector4>("Vector4",
        sol::constructors<Vector4(), Vector4(float, float, float, float), Vector4(const Vector3&, float)>(),
        "x", &Vector4::x,
        "y", &Vector4::y,
        "z", &Vector4::z,
        "w", &Vector4::w,
        "length", &Vector4::length,
        "squared", &Vector4::squared,
        "normalized", &Vector4::normalized,
        "absolute", &Vector4::absolute,
        "dot", &Vector4::dot,
        "clamped", &Vector4::clamped,
        "approximately", &Vector4::approximately,
        "minimum", &Vector4::minimum,
        "maximum", &Vector4::maximum,
        sol::meta_function::addition, [](const Vector4& alpha, const Vector4& beta) { return alpha + beta; },
        sol::meta_function::subtraction, [](const Vector4& alpha, const Vector4& beta) { return alpha - beta; },
        sol::meta_function::unary_minus, [](const Vector4& alpha) { return -alpha; },
        sol::meta_function::equal_to, [](const Vector4& alpha, const Vector4& beta) { return alpha == beta; },
        sol::meta_function::multiplication, sol::overload(
            [](const Vector4& alpha, const float scalar) { return alpha * scalar; },
            [](const float scalar, const Vector4& beta) { return beta * scalar; }
        ),
        sol::meta_function::to_string, [](const Vector4& alpha) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "Vector4(%f, %f, %f, %f)", static_cast<double>(alpha.x), static_cast<double>(alpha.y), static_cast<double>(alpha.z), static_cast<double>(alpha.w));
            return std::string(buffer);
        }
    );

    view.new_usertype<Quaternion>("Quaternion",
        sol::constructors<Quaternion(), Quaternion(float, float, float, float)>(),
        "x", &Quaternion::x,
        "y", &Quaternion::y,
        "z", &Quaternion::z,
        "w", &Quaternion::w,
        "length", [](const Quaternion& quaternion) { return static_cast<float>(quaternion.length()); },
        "dot", [](const Quaternion& alpha, const Quaternion& beta) { return static_cast<float>(alpha.dot(beta)); },
        "normalized", &Quaternion::normalized,
        "conjugate", &Quaternion::conjugate,
        "inverse", &Quaternion::inverse,
        "interpolate", &Quaternion::interpolate,
        "euler", static_cast<Vector3 (Quaternion::*)() const>(&Quaternion::euler),
        "between", &Quaternion::between,
        "look", &Quaternion::look,
        "around", &Quaternion::around,
        sol::meta_function::addition, [](const Quaternion& alpha, const Quaternion& beta) { return alpha + beta; },
        sol::meta_function::subtraction, [](const Quaternion& alpha, const Quaternion& beta) { return alpha - beta; },
        sol::meta_function::equal_to, [](const Quaternion& alpha, const Quaternion& beta) { return alpha == beta; },
        sol::meta_function::multiplication, sol::overload(
            [](const Quaternion& alpha, const float scalar) { return alpha * scalar; },
            [](const float scalar, const Quaternion& beta) { return beta * scalar; },
            [](const Quaternion& alpha, const Quaternion& beta) { return alpha * beta; }
        ),
        sol::meta_function::to_string, [](const Quaternion& alpha) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "Quaternion(%f, %f, %f, %f)", static_cast<double>(alpha.x), static_cast<double>(alpha.y), static_cast<double>(alpha.z), static_cast<double>(alpha.w));
            return std::string(buffer);
        }
    );

    view.new_usertype<Matrix3>("Matrix3",
        sol::constructors<Matrix3(), Matrix3(float)>(),
        "transposed", &Matrix3::transposed,
        "determinant", [](const Matrix3& matrix) { return static_cast<float>(matrix.determinant()); },
        "inverted", &Matrix3::inverted,
        "approximately", &Matrix3::approximately,
        "translation", &Matrix3::translation,
        "rotation", [](const Matrix3& matrix) { return static_cast<float>(matrix.rotation()); },
        "scale", static_cast<Vector2 (Matrix3::*)() const>(&Matrix3::scale),
        "identity", [] { return Matrix3::IDENTITY; },
        "translate", &Matrix3::translate,
        "rotate", &Matrix3::rotate,
        "factor", static_cast<Matrix3 (*)(const Vector2&)>(&Matrix3::scale),
        sol::meta_function::addition, [](const Matrix3& alpha, const Matrix3& beta) { return alpha + beta; },
        sol::meta_function::subtraction, [](const Matrix3& alpha, const Matrix3& beta) { return alpha - beta; },
        sol::meta_function::multiplication, [](const Matrix3& alpha, const Matrix3& beta) { return alpha * beta; },
        sol::meta_function::equal_to, [](const Matrix3& alpha, const Matrix3& beta) { return alpha == beta; },
        sol::meta_function::to_string, [](const Matrix3& matrix) {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer),
                "Matrix3([[ %f, %f, %f ], [ %f, %f, %f ], [ %f, %f, %f ]])",
                static_cast<double>(matrix(0,0)), static_cast<double>(matrix(0,1)), static_cast<double>(matrix(0,2)),
                static_cast<double>(matrix(1,0)), static_cast<double>(matrix(1,1)), static_cast<double>(matrix(1,2)),
                static_cast<double>(matrix(2,0)), static_cast<double>(matrix(2,1)), static_cast<double>(matrix(2,2)));
            return std::string(buffer);
        }
    );

    view.new_usertype<Matrix4>("Matrix4",
        sol::constructors<Matrix4(), Matrix4(float)>(),
        "transposed", &Matrix4::transposed,
        "inverted", &Matrix4::inverted,
        "decompose", &Matrix4::decompose,
        "identity", [] { return Matrix4::IDENTITY; },
        "translate", &Matrix4::translate,
        "rotate", &Matrix4::rotate,
        "scale", &Matrix4::scale,
        "orthographic", &Matrix4::orthographic,
        "perspective", &Matrix4::perspective,
        "look", &Matrix4::look,
        sol::meta_function::addition, [](const Matrix4& alpha, const Matrix4& beta) { return alpha + beta; },
        sol::meta_function::subtraction, [](const Matrix4& alpha, const Matrix4& beta) { return alpha - beta; },
        sol::meta_function::multiplication, [](const Matrix4& alpha, const Matrix4& beta) { return alpha * beta; },
        sol::meta_function::equal_to, [](const Matrix4& alpha, const Matrix4& beta) {
            for (int row = 0; row < 4; row++)
                for (int column = 0; column < 4; column++)
                    if (alpha(row, column) != beta(row, column)) return false;
            return true;
        },
        sol::meta_function::to_string, [](const Matrix4& alpha) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "Matrix4([[ %f, %f, %f, %f ], ...])",
                static_cast<double>(alpha(0,0)), static_cast<double>(alpha(0,1)), static_cast<double>(alpha(0,2)), static_cast<double>(alpha(0,3)));
            return std::string(buffer);
        }
    );

    view.new_usertype<Transform>("Transform",
        sol::constructors<Transform(), Transform(const Vector3&, const Quaternion&, const Vector3&)>(),
        "translation", &Transform::translation,
        "rotation", &Transform::rotation,
        "scale", &Transform::scale,
        "matrix", &Transform::matrix,
        "forward", &Transform::forward,
        "right", &Transform::right,
        "up", &Transform::up,
        "inversed", &Transform::inversed,
        "interpolate", &Transform::interpolate,
        "point", &Transform::point,
        "direction", &Transform::direction,
        "approximately", &Transform::approximately,
        "from", &Transform::from,
        sol::meta_function::equal_to, [](const Transform& alpha, const Transform& beta) { return alpha == beta; },
        sol::meta_function::to_string, [](const Transform& alpha) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "Transform(translation=(%f, %f, %f))", static_cast<double>(alpha.translation.x), static_cast<double>(alpha.translation.y), static_cast<double>(alpha.translation.z));
            return std::string(buffer);
        }
    );
}