#include "../include/source_position.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

// Base64 VLQ encoding for source map format
namespace {
    // Base64 character set for source map encoding
    const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Convert a number to base64 VLQ format
    std::string toBase64VLQ(int value) {
        // Handle negative numbers
        bool negative = value < 0;
        value = std::abs(value);

        // For negative numbers, the least significant bit is 1
        uint32_t vlq = negative ? (value << 1) | 1 : (value << 1);

        std::string result;
        do {
            uint32_t digit = vlq & 0x1F; // 5 bits per char
            vlq >>= 5;

            // If there are more bits, set the continuation bit
            if (vlq > 0) {
                digit |= 0x20;
            }

            result += base64Chars[digit];
        } while (vlq > 0);

        return result;
    }
} // namespace

// Generate source map in V3 format
std::string SourceMap::generate() const {
    // Extract and sort all generated positions
    std::vector<uint32_t> positions;
    positions.reserve(mappings_.size());

    for (const auto &mapping: mappings_) {
        positions.push_back(mapping.first);
    }

    // Sort by line, then by column
    std::sort(positions.begin(), positions.end(), [](uint32_t a, uint32_t b) {
        SourcePosition posA(a);
        SourcePosition posB(b);

        if (posA.getLine() != posB.getLine()) {
            return posA.getLine() < posB.getLine();
        }
        return posA.getColumn() < posB.getColumn();
    });

    // Collect unique source files
    std::vector<std::string> sourceFiles;
    for (const auto &source: sources_) {
        if (std::find(sourceFiles.begin(), sourceFiles.end(), source.second) == sourceFiles.end()) {
            sourceFiles.push_back(source.second);
        }
    }

    // Generate the mappings string
    std::stringstream mappings;

    int32_t previousGeneratedLine = 0;
    int32_t previousGeneratedColumn = 0;
    int32_t previousOriginalLine = 0;
    int32_t previousOriginalColumn = 0;
    int32_t previousSource = 0;

    for (size_t i = 0; i < positions.size(); ++i) {
        SourcePosition genPos(positions[i]);
        SourcePosition origPos(mappings_.at(positions[i]));

        // Find source file index
        int32_t sourceIndex = -1;
        std::string sourceFile = "";

        auto sourceIt = sources_.find(positions[i]);
        if (sourceIt != sources_.end()) {
            sourceFile = sourceIt->second;
            auto it = std::find(sourceFiles.begin(), sourceFiles.end(), sourceFile);
            if (it != sourceFiles.end()) {
                sourceIndex = static_cast<int32_t>(it - sourceFiles.begin());
            }
        }

        // Start a new line if needed
        if (genPos.getLine() > previousGeneratedLine) {
            for (int32_t j = previousGeneratedLine; j < genPos.getLine(); ++j) {
                mappings << ";";
            }
            previousGeneratedColumn = 0;
        } else if (i > 0) {
            mappings << ",";
        }

        // 1. Generated column delta
        mappings << toBase64VLQ(genPos.getColumn() - previousGeneratedColumn);
        previousGeneratedColumn = genPos.getColumn();

        // Only include source/original position if we have that information
        if (sourceIndex >= 0) {
            // 2. Source index delta
            mappings << toBase64VLQ(sourceIndex - previousSource);
            previousSource = sourceIndex;

            // 3. Original line delta
            mappings << toBase64VLQ(origPos.getLine() - previousOriginalLine);
            previousOriginalLine = origPos.getLine();

            // 4. Original column delta
            mappings << toBase64VLQ(origPos.getColumn() - previousOriginalColumn);
            previousOriginalColumn = origPos.getColumn();
        }

        if (genPos.getLine() > previousGeneratedLine) {
            previousGeneratedLine = genPos.getLine();
        }
    }

    // Build the source map JSON
    std::stringstream json;
    json << "{\n";
    json << "  \"version\": 3,\n";
    json << "  \"file\": \"\",\n";
    json << "  \"sourceRoot\": \"\",\n";
    json << "  \"sources\": [";

    for (size_t i = 0; i < sourceFiles.size(); ++i) {
        if (i > 0)
            json << ", ";
        json << "\"" << sourceFiles[i] << "\"";
    }

    json << "],\n";
    json << "  \"names\": [],\n";
    json << "  \"mappings\": \"" << mappings.str() << "\"\n";
    json << "}\n";

    return json.str();
}
