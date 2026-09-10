#include "doctest.h"

#include "core/constants.hpp"
#include "core/piece.hpp"

using namespace tetris;

TEST_CASE("shape tables have 7 pieces, 4 rotations, 4 cells") {
    CHECK(kShapeCount == 7);
    for (int s = 0; s < kShapeCount; ++s) {
        for (int r = 0; r < 4; ++r) {
            CHECK(kShapes[s][r].size() == 4);
        }
    }
}

TEST_CASE("piece spawns at (3,0) rotation 0") {
    Piece p(ShapeId::T);
    CHECK(p.x() == kSpawnX);
    CHECK(p.y() == kSpawnY);
    CHECK(p.rotation() == 0);
    CHECK(p.shape() == ShapeId::T);
}

TEST_CASE("piece cells are shape offsets translated by position") {
    Piece p(ShapeId::T);  // rotation 0: (1,0),(0,1),(1,1),(2,1)
    p.setPos(4, 2);
    auto cells = p.cells();
    // Expected absolute cells = offsets + (4,2)
    CHECK(cells[0].x == 5); CHECK(cells[0].y == 2);
    CHECK(cells[1].x == 4); CHECK(cells[1].y == 3);
    CHECK(cells[2].x == 5); CHECK(cells[2].y == 3);
    CHECK(cells[3].x == 6); CHECK(cells[3].y == 3);
}

TEST_CASE("rotation wraps modulo 4 in both directions") {
    Piece p(ShapeId::J);
    p.setRotation(5);
    CHECK(p.rotation() == 1);
    p.setRotation(-1);
    CHECK(p.rotation() == 3);
    p.setRotation(4);
    CHECK(p.rotation() == 0);
}

TEST_CASE("kick tables by shape class") {
    int n = 0;
    Piece(ShapeId::I).kicks(n);
    CHECK(n == 6);
    Piece(ShapeId::O).kicks(n);
    CHECK(n == 1);  // O never really kicks
    Piece(ShapeId::T).kicks(n);
    CHECK(n == 6);  // JLSTZ table
    Piece(ShapeId::S).kicks(n);
    CHECK(n == 6);
}

TEST_CASE("O piece is identical across all rotations") {
    Piece p(ShapeId::O);
    auto r0 = p.cellsAt(0, 0, 0);
    for (int r = 1; r < 4; ++r) {
        auto rn = p.cellsAt(0, 0, r);
        for (int i = 0; i < 4; ++i) {
            CHECK(rn[i].x == r0[i].x);
            CHECK(rn[i].y == r0[i].y);
        }
    }
}
