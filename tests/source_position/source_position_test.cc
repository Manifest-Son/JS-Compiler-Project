#include "../../include/source_position.h"
#include <gtest/gtest.h>
#include <memory>
#include "../../include/ast.h"

class SourcePositionTest : public ::testing::Test {};

// Test the bit-packed position class
TEST_F(SourcePositionTest, BitPackingTest) {
    // Test packing and unpacking of line/column
    SourcePosition pos1(42, 10);
    EXPECT_EQ(pos1.getLine(), 42u);
    EXPECT_EQ(pos1.getColumn(), 10u);

    // Test maximum values
    SourcePosition pos2((1 << 20) - 1, (1 << 12) - 1);
    EXPECT_EQ(pos2.getLine(), (1u << 20) - 1); // 1048575 (max line)
    EXPECT_EQ(pos2.getColumn(), (1u << 12) - 1); // 4095 (max column)

    // Test overflow handling (should mask to valid range)
    SourcePosition pos3(1 << 25, 1 << 15); // Exceeds allowed bits
    EXPECT_EQ(pos3.getLine(), (1u << 20) - 1); // Should be max value
    EXPECT_EQ(pos3.getColumn(), (1u << 12) - 1); // Should be max value

    // Test direct packing constructor
    uint32_t packed = (42u << 12) | 10u;
    SourcePosition pos4(packed);
    EXPECT_EQ(pos4.getLine(), 42u);
    EXPECT_EQ(pos4.getColumn(), 10u);

    // Test serialization to string
    EXPECT_EQ(pos1.toString(), "42:10");
}

// Test the source range class
TEST_F(SourcePositionTest, SourceRangeTest) {
    SourcePosition start(10, 5);
    SourcePosition end(10, 15);

    SourceRange range(start, end);
    EXPECT_EQ(range.start().getLine(), 10u);
    EXPECT_EQ(range.start().getColumn(), 5u);
    EXPECT_EQ(range.end().getLine(), 10u);
    EXPECT_EQ(range.end().getColumn(), 15u);

    // Test string representation
    EXPECT_EQ(range.toString(), "10:5-10:15");
}

// Test AST node source position integration
TEST_F(SourcePositionTest, ASTNodePositionTest) {
    // Create a mock token
    Token token;
    token.type = TokenType::NUMBER;
    token.lexeme = "42";
    token.line = 10;
    token.column = 5;

    // Create an AST node with source position
    auto literal = std::make_shared<LiteralExpr>(token);
    literal->setPosition(token);

    // Verify the position was set correctly
    EXPECT_EQ(literal->start_pos.getLine(), 10u);
    EXPECT_EQ(literal->start_pos.getColumn(), 5u);
    EXPECT_EQ(literal->end_pos.getLine(), 10u);
    EXPECT_EQ(literal->end_pos.getColumn(), token.lexeme.length() + 5u);

    // Test source range
    SourceRange range = literal->getSourceRange();
    EXPECT_EQ(range.toString(), "10:5-10:7");
}

// Test source map generation
TEST_F(SourcePositionTest, SourceMapGenerationTest) {
    SourceMap sourceMap;

    // Add some mappings
    sourceMap.addMapping(1, 0, 5, 10, "source1.js");
    sourceMap.addMapping(1, 5, 5, 15, "source1.js");
    sourceMap.addMapping(2, 0, 6, 0, "source1.js");
    sourceMap.addMapping(2, 10, 7, 5, "source2.js");

    // Test source position lookup
    SourcePosition origPos = sourceMap.getOriginalPosition(1, 0);
    EXPECT_EQ(origPos.getLine(), 5u);
    EXPECT_EQ(origPos.getColumn(), 10u);

    // Test source file lookup
    std::string sourceFile = sourceMap.getSourceFile(2, 10);
    EXPECT_EQ(sourceFile, "source2.js");

    // Test source map generation contains required fields
    std::string sourceMapJson = sourceMap.generate();
    EXPECT_TRUE(sourceMapJson.find("\"version\": 3") != std::string::npos);
    EXPECT_TRUE(sourceMapJson.find("\"sources\": [") != std::string::npos);
    EXPECT_TRUE(sourceMapJson.find("\"source1.js\"") != std::string::npos);
    EXPECT_TRUE(sourceMapJson.find("\"source2.js\"") != std::string::npos);
    EXPECT_TRUE(sourceMapJson.find("\"mappings\": \"") != std::string::npos);
}
