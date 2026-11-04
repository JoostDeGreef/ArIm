#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <unordered_set>
#include <set>

#include "tokenizer.h"

using namespace std;

Tokenizer::Tokenizer()
    : tokenizedInputSequence()
    , nextUnusedToken(256)
    , tokenPairCountAndPositions()
    , tokenPairReplacements()
    , tokenRelationships()
{}

Tokenizer::Tokenizer(SQLite::DB & db)
    : Tokenizer()
{
    LoadDataFromDB(db);
}

void Tokenizer::Process(const std::string& input)
{
    // WARNING:
    //   this is just a placeholder of what should be executed,
    //   not in any way proper or ready for review yet.
    auto firstTokenToUpdate = tokenizedInputSequence.last();
    FeedInput(input);
    UpdateTokenPairCountAndPositions(firstTokenToUpdate);
    ApplyExistingTokenPairReplacements();
    OptimizeTokenLength(settings.tokenDictionary);
    CalculateTokenRelationships(settings.relationDepth);
}

void Tokenizer::FeedInput(const std::string& input)
{
    bool separated = true;
    for(uint8_t c: input)
    {
        if(isspace(c))
        {
            separated = true;
        }
        else
        {
            separated = false;
            tokenizedInputSequence.emplace_back(c,separated);
        }
    }
}

void Tokenizer::UpdateTokenPairCountAndPositions(linked_list<Token>::iterator position)
{
    if(position == tokenizedInputSequence.end())
    {
        return;
    }
    auto prevPosition = position;
    for(++position; position != tokenizedInputSequence.end(); ++position)
    {
        const Token & left = *prevPosition;
        const Token & right = *position;
        TokenPair tokenPair(left, right);
        auto & countAndPosition = tokenPairCountAndPositions[tokenPair];
        countAndPosition.count++;
        countAndPosition.positions.emplace(prevPosition);
        prevPosition = position;
    }
}

void Tokenizer::ApplyExistingTokenPairReplacements()
{
    auto position = tokenizedInputSequence.begin();
    if(position == tokenizedInputSequence.end() || tokenPairReplacements.empty())
    {
        return;
    }
    auto prevPosition = position;
    for(++position; position != tokenizedInputSequence.end();)
    {
        const Token & left = *prevPosition;
        const Token & right = *position;
        TokenPair tokenPair(left, right);
        auto replacementIt = tokenPairReplacements.find(tokenPair);
        if(replacementIt != tokenPairReplacements.end())
        {
            const Token & replacement = replacementIt->second;
            position = prevPosition = tokenizedInputSequence.replace_two(prevPosition, replacement);
            auto countAndPositionIter = tokenPairCountAndPositions.find(tokenPair);
            if(countAndPositionIter != tokenPairCountAndPositions.end())
            {
                countAndPositionIter->second.count--;
                if(countAndPositionIter->second.count <= 0)
                {
                    tokenPairCountAndPositions.erase(countAndPositionIter);
                }
                else
                {
                    auto & positions = countAndPositionIter->second.positions;
                    auto posIt = std::find(positions.begin(), positions.end(), position);
                    if(posIt != positions.end())
                    {
                        positions.erase(posIt); 
                    }
                }
            }
        }
        else
        {
            prevPosition = position;
        }
        ++position;
    }
}

void Tokenizer::OptimizeTokenLength(size_t maxUsedTokensCount)
{
    if(tokenPairCountAndPositions.empty())
    {
        return;
    }
    auto RemoveTokenPairSequencePosition = [&](linked_list<Token>::iterator position)
    {
        if(position == tokenizedInputSequence.end())
        {
            return;
        }
        auto nextPosition = position;
        ++nextPosition;
        auto & left = *position;
        auto & right = *nextPosition;
        auto & countAndPosition = tokenPairCountAndPositions[TokenPair(left, right)];
        --countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.erase(position);
    };
    auto AddTokenPairSequencePosition = [&](linked_list<Token>::iterator position)
    {
        if(position == tokenizedInputSequence.end())
        {
            return;
        }
        auto nextPosition = position;
        ++nextPosition;
        auto & left = *position;
        auto & right = *nextPosition;
        auto & countAndPosition = tokenPairCountAndPositions[TokenPair(left, right)];
        ++countAndPosition.count;
        auto & positions = countAndPosition.positions;
        positions.emplace(position);
    };
    while(tokenPairReplacements.size() + 256 < maxUsedTokensCount)
    {
        auto iterMax = tokenPairCountAndPositions.begin();
        if(iterMax == tokenPairCountAndPositions.end())
        {
            break;
        }
        for(auto iter = tokenPairCountAndPositions.begin(); iter != tokenPairCountAndPositions.end(); ++iter)
        {
            if(iter->second.count > iterMax->second.count)
            {
                iterMax = iter;
            }
        }
        const auto & tokenPair = iterMax->first;
        const auto & left = tokenPair.left;
        const auto & right = tokenPair.right;
        auto replacementIt = tokenPairReplacements.find(tokenPair);
        if(replacementIt == tokenPairReplacements.end())
        {
            Token newToken = nextUnusedToken++;
            if(left.isSeparator())
            {
                newToken.setSeparator();
            }
            tokenPairReplacements[tokenPair] = Token(newToken);
        }
        else
        {
            // an existing replacement for this token pair already exists, weird, but let's use it
        }
        const auto & replacement = tokenPairReplacements[tokenPair];
        while(!iterMax->second.positions.empty())
        {
            auto position = *iterMax->second.positions.begin();
            auto nextPosition = position;
            ++nextPosition;
            if(nextPosition != tokenizedInputSequence.end() && *position == left && *nextPosition == right)
            {
                auto prevPosition = position;
                --prevPosition;
                auto nextNextPosition = nextPosition;
                ++nextNextPosition;
                RemoveTokenPairSequencePosition(prevPosition);
                RemoveTokenPairSequencePosition(position);
                RemoveTokenPairSequencePosition(nextPosition);
                position = tokenizedInputSequence.replace_two(position, replacement);
                AddTokenPairSequencePosition(prevPosition);
                AddTokenPairSequencePosition(position);
            }
        }
    }
}

void Tokenizer::CalculateTokenRelationships(size_t depth)
{

}

// Store tokenizer state to a database
void Tokenizer::StoreTokenizerToDB(SQLite::DB & db) const
{
    // store settings
    // store tokenizedInputSequence
    // store nextUnusedToken
    // store tokenPairCountAndPositions
    // store tokenPairReplacements
    // store tokenRelationships
}
    
void Tokenizer::LoadDataFromDB(SQLite::DB & db)
{
    // load settings, check version
    // load tokenizedInputSequence
    // load nextUnusedToken
    // load tokenPairCountAndPositions
    // load tokenPairReplacements
    // load tokenRelationships
}
