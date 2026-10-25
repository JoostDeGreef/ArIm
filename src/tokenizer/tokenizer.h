#pragma once

#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "token.h"
#include "tokenpair.h"

typedef std::unordered_set<TokenPair,TokenPair::HashFunction> TokenPairSet;
typedef std::vector<TokenPairSet> TokenPairSets;
typedef std::vector<TokenPair> TokenPairVector;
typedef std::vector<Token::Data> TokenSequence;
typedef std::vector<TokenSequence> TokenSequences;
typedef std::unordered_map<Token::Data, Token> TokensMap;
typedef std::unordered_map<Token::Data, Token::Data, Token::HashFunction> TokensMappedByCount;
typedef std::unordered_map<Token::Data, size_t> TokenCountMap;

/*
 * Tokenizer class for tokenizing input text into tokens. 
 *
 * Normal operation steps:
 *   1. Feed input text to the tokenizer
 *   2. Optimize token length until the maximum (used) token count is reached or no more text is available
 *   3. Calculate token relationships
 *   4. Store the results
 */
class Tokenizer 
{   
public:
    Tokenizer();
    // This class stores a lot of data, so we don't want to allow copying or moving it around.
    Tokenizer(const Tokenizer&) = delete;
    Tokenizer(Tokenizer&&) = delete;
    Tokenizer& operator=(const Tokenizer&) = delete;
    Tokenizer& operator=(Tokenizer&&) = delete;

    // 1. add text to the tokenizer. can be called repeatedly
    void FeedInput(const std::string& input); 
    // 2. combine smaller tokens until the list of tokens used reaches n, in blocks of step.
    //    lower number of steps gives more accurate result, but takes longer
    void OptimizeTokenLength(size_t n = 1024, size_t step = 5);
    // 3. store all token relations found in input up to depth 
    void CalculateTokenRelationships(size_t depth = 5); 
    // 4. store token sequences and token relationships to a database
    void StoreCalculatedDataToDB(const std::string & filename) const;

private:
    // all tokens, intermediate nodes and root nodes
    TokensMap tokens; 
    Token::Data nextToken = 0;
    size_t empty_tokens_count = 0;

    // input data split in token sequences
    TokenSequences sequences;

    // the relationship between any token and the n tokens before it
    std::unordered_map<Token::Data,std::vector<TokenCountMap>> tokenRelations;

    /*
     *   internal functions
     */

    // return the number of used tokens
    size_t GetUsedTokensCount() const;

    // create all token pairs for one sequence
    TokenPairSet CreateTokenPairsForSequence(const TokenSequence & sequence) const;

    // create all tokenpairs for each sequence
    TokenPairSets CreateTokenPairsForSequences(const TokenSequences & sequences) const;

    // create all tokenpairs for all sequences
    TokenPairSet FlattenSequenceTokenPairs(const TokenPairSets & tokenPairSets);

    // collect the tokenpairs found in all sequences, and report the top n pairs found
    TokenPairVector GetTopTokenPairs(size_t n, const TokenPairSet & tokenPairSet) const;

    // replace a tokenpair with a token in sequence and update the tokens count. the new token should already be in tokens
    void ReplaceTokenPairWithToken(TokenSequence & sequence, const TokenPair & tokenPair,const Token::Data newToken);

    // update sequences, sequencesTokenPairs, flattenedTokenPairs and tokens by merging any pairs found in pairsForMerging 
    void MergeSpecificPairs(const TokenPairVector & pairsForMerging, TokenPairSets & sequencesTokenPairs, TokenPairSet & flattenedTokenPairs);

    // get the originat text sequence the token represents
    std::string GetTokenTextSequence(const Token::Data & token) const;

    // create a mapping for all used tokens to new token values starting at 0, with decreasing count
    // this is usefull when storing to db, where smaller numbers migth take less storage size
    TokensMappedByCount CreateMappingForTokensByCount() const;
};
