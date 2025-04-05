#ifndef SOURCE_POSITION_H
#define SOURCE_POSITION_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Efficient source position storage using bit packing
// Format: 32 bits total
// - 20 bits for line number (supports up to 1,048,575 lines)
// - 12 bits for column number (supports up to 4,095 columns per line)
class SourcePosition {
public:
    // Default constructor creates invalid position
    SourcePosition() : packed_position_(0) {}

    // Create position from line and column
    SourcePosition(uint32_t line, uint32_t column) { setPosition(line, column); }

    // Create position from packed value
    explicit SourcePosition(uint32_t packed) : packed_position_(packed) {}

    // Getters and setters
    uint32_t getLine() const { return (packed_position_ >> COLUMN_BITS) & LINE_MASK; }

    uint32_t getColumn() const { return packed_position_ & COLUMN_MASK; }

    void setPosition(uint32_t line, uint32_t column) {
        // Ensure values fit within bit ranges
        line = line & LINE_MASK;
        column = column & COLUMN_MASK;

        // Pack into single 32-bit value
        packed_position_ = (line << COLUMN_BITS) | column;
    }

    uint32_t getPacked() const { return packed_position_; }

    bool isValid() const { return packed_position_ != 0; }

    std::string toString() const {
        if (!isValid())
            return "unknown";
        return std::to_string(getLine()) + ":" + std::to_string(getColumn());
    }

    // Return a pointer to the internal 32-bit value for direct memory access
    const uint32_t *data() const { return &packed_position_; }

private:
    // Bit allocation constants
    static constexpr uint32_t COLUMN_BITS = 12;
    static constexpr uint32_t LINE_BITS = 20;
    static constexpr uint32_t COLUMN_MASK = (1 << COLUMN_BITS) - 1;
    static constexpr uint32_t LINE_MASK = (1 << LINE_BITS) - 1;

    // The packed position data
    uint32_t packed_position_;
};

// Source range represents a range of positions (start to end)
class SourceRange {
public:
    SourceRange() = default;

    SourceRange(const SourcePosition &start, const SourcePosition &end) : start_(start), end_(end) {}

    const SourcePosition &start() const { return start_; }
    const SourcePosition &end() const { return end_; }

    void setStart(const SourcePosition &pos) { start_ = pos; }
    void setEnd(const SourcePosition &pos) { end_ = pos; }

    bool isValid() const { return start_.isValid() && end_.isValid(); }

    std::string toString() const {
        if (!isValid())
            return "unknown";
        return start_.toString() + "-" + end_.toString();
    }

private:
    SourcePosition start_;
    SourcePosition end_;
};

// Source map for generating debugging information
class SourceMap {
public:
    SourceMap() = default;

    // Add a mapping between generated position and original source position
    void addMapping(uint32_t generated_line, uint32_t generated_column, uint32_t original_line,
                    uint32_t original_column, const std::string &source_file = "") {
        SourcePosition gen_pos(generated_line, generated_column);
        SourcePosition orig_pos(original_line, original_column);

        mappings_[gen_pos.getPacked()] = orig_pos.getPacked();

        if (!source_file.empty()) {
            sources_[gen_pos.getPacked()] = source_file;
        }
    }

    // Get original position for a generated position
    SourcePosition getOriginalPosition(uint32_t generated_line, uint32_t generated_column) const {
        SourcePosition gen_pos(generated_line, generated_column);
        auto it = mappings_.find(gen_pos.getPacked());

        if (it != mappings_.end()) {
            return SourcePosition(it->second);
        }

        return SourcePosition(); // Invalid position
    }

    // Get source file for a generated position
    std::string getSourceFile(uint32_t generated_line, uint32_t generated_column) const {
        SourcePosition gen_pos(generated_line, generated_column);
        auto it = sources_.find(gen_pos.getPacked());

        if (it != sources_.end()) {
            return it->second;
        }

        return ""; // Unknown source
    }

    // Generate source map in a compressed format (V3 source map format)
    std::string generate() const;

private:
    // Maps from generated position to original position (both packed)
    std::unordered_map<uint32_t, uint32_t> mappings_;

    // Maps from generated position to source file
    std::unordered_map<uint32_t, std::string> sources_;
};

#endif // SOURCE_POSITION_H
