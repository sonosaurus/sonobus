// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SoundFlipAPI.h"

//==============================================================================
SoundFlipAPI::SoundFlipAPI(SoundFlipAuth& authRef)
    : auth(authRef)
{
}

SoundFlipAPI::~SoundFlipAPI()
{
}

//==============================================================================
var SoundFlipAPI::makeRequest(const String& endpoint,
                               const String& method,
                               const var& body)
{
    lastError.clear();
    lastStatusCode = 0;
    
    String headers = "Content-Type: application/json\r\n";
    headers += "Authorization: Bearer " + auth.getAccessToken();
    
    URL url(apiBaseUrl + endpoint);
    
    bool hasBody = !body.isVoid() && (method == "POST" || method == "PUT" || method == "PATCH");
    if (hasBody)
    {
        url = url.withPOSTData(JSON::toString(body));
    }
    
    auto paramHandling = (method == "GET") 
        ? URL::ParameterHandling::inAddress 
        : URL::ParameterHandling::inPostData;
    
    // Build options chain - can't reassign due to const member
    std::unique_ptr<InputStream> stream;
    
    if (method != "GET" && method != "POST")
    {
        // Need custom HTTP method
        stream = url.createInputStream(
            URL::InputStreamOptions(paramHandling)
                .withExtraHeaders(headers)
                .withConnectionTimeoutMs(30000)
                .withStatusCode(&lastStatusCode)
                .withHttpRequestCmd(method)
        );
    }
    else
    {
        stream = url.createInputStream(
            URL::InputStreamOptions(paramHandling)
                .withExtraHeaders(headers)
                .withConnectionTimeoutMs(30000)
                .withStatusCode(&lastStatusCode)
        );
    }
    
    if (stream == nullptr)
    {
        // Check if token might be expired
        if (auth.isTokenExpired())
        {
            // Try to refresh and retry
            if (auth.refreshAccessToken())
            {
                headers = "Content-Type: application/json\r\n";
                headers += "Authorization: Bearer " + auth.getAccessToken();
                
                URL retryUrl(apiBaseUrl + endpoint);
                if (hasBody)
                {
                    retryUrl = retryUrl.withPOSTData(JSON::toString(body));
                }
                
                if (method != "GET" && method != "POST")
                {
                    stream = retryUrl.createInputStream(
                        URL::InputStreamOptions(paramHandling)
                            .withExtraHeaders(headers)
                            .withConnectionTimeoutMs(30000)
                            .withStatusCode(&lastStatusCode)
                            .withHttpRequestCmd(method)
                    );
                }
                else
                {
                    stream = retryUrl.createInputStream(
                        URL::InputStreamOptions(paramHandling)
                            .withExtraHeaders(headers)
                            .withConnectionTimeoutMs(30000)
                            .withStatusCode(&lastStatusCode)
                    );
                }
            }
        }
        
        if (stream == nullptr)
        {
            lastError = "Network error - could not connect";
            return var();
        }
    }
    
    String response = stream->readEntireStreamAsString();
    
    var jsonResponse = JSON::parse(response);
    
    if (jsonResponse.isVoid())
    {
        lastError = "Invalid JSON response";
        return var();
    }
    
    if (jsonResponse.hasProperty("error"))
    {
        lastError = jsonResponse["error"].toString();
        return var();
    }
    
    return jsonResponse;
}

//==============================================================================
SoundFlipAPI::Session SoundFlipAPI::parseSession(const var& json)
{
    Session session;
    session.id = json["id"].toString();
    session.name = json["name"].toString();
    session.description = json["description"].toString();
    session.hostUserId = json["hostUserId"].toString();
    session.connectionCode = json["connectionCode"].toString();
    session.status = json["status"].toString();
    session.createdAt = (int64)json["createdAt"];
    session.updatedAt = (int64)json["updatedAt"];
    return session;
}

SoundFlipAPI::Stem SoundFlipAPI::parseStem(const var& json)
{
    Stem stem;
    stem.id = json["id"].toString();
    stem.sessionId = json["sessionId"].toString();
    stem.userId = json["userId"].toString();
    stem.fileName = json["fileName"].toString();
    stem.fileUrl = json["fileUrl"].toString();
    stem.fileSize = (int64)json["fileSize"];
    stem.status = json["status"].toString();
    stem.createdAt = (int64)json["createdAt"];
    return stem;
}

//==============================================================================
SoundFlipAPI::Session SoundFlipAPI::createSession(const String& name, const String& description)
{
    DynamicObject::Ptr body = new DynamicObject();
    body->setProperty("name", name);
    if (description.isNotEmpty())
        body->setProperty("description", description);
    
    var response = makeRequest("/sessions", "POST", var(body.get()));
    
    if (response.isVoid())
        return Session();
    
    return parseSession(response);
}

SoundFlipAPI::Session SoundFlipAPI::getSession(const String& sessionId)
{
    var response = makeRequest("/sessions/" + sessionId);
    
    if (response.isVoid())
        return Session();
    
    return parseSession(response);
}

SoundFlipAPI::Session SoundFlipAPI::joinSession(const String& inviteCode)
{
    DynamicObject::Ptr body = new DynamicObject();
    body->setProperty("inviteCode", inviteCode);
    
    var response = makeRequest("/sessions/join", "POST", var(body.get()));
    
    if (response.isVoid())
        return Session();
    
    return parseSession(response);
}

bool SoundFlipAPI::leaveSession(const String& sessionId)
{
    var response = makeRequest("/sessions/" + sessionId + "/leave", "POST");
    return !response.isVoid();
}

bool SoundFlipAPI::endSession(const String& sessionId)
{
    var response = makeRequest("/sessions/" + sessionId + "/end", "POST");
    return !response.isVoid();
}

Array<SoundFlipAPI::Session> SoundFlipAPI::listSessions()
{
    Array<Session> sessions;
    
    var response = makeRequest("/sessions");
    
    if (response.isVoid() || !response.isArray())
        return sessions;
    
    for (int i = 0; i < response.size(); ++i)
    {
        sessions.add(parseSession(response[i]));
    }
    
    return sessions;
}

//==============================================================================
String SoundFlipAPI::getUploadUrl(const String& sessionId, const String& fileName, int64 fileSize)
{
    DynamicObject::Ptr body = new DynamicObject();
    body->setProperty("fileName", fileName);
    body->setProperty("fileSize", fileSize);
    
    var response = makeRequest("/sessions/" + sessionId + "/stems/upload-url", "POST", var(body.get()));
    
    if (response.isVoid())
        return {};
    
    return response["uploadUrl"].toString();
}

SoundFlipAPI::Stem SoundFlipAPI::completeUpload(const String& sessionId, const String& uploadId)
{
    DynamicObject::Ptr body = new DynamicObject();
    body->setProperty("uploadId", uploadId);
    
    var response = makeRequest("/sessions/" + sessionId + "/stems/complete", "POST", var(body.get()));
    
    if (response.isVoid())
        return Stem();
    
    return parseStem(response);
}

Array<SoundFlipAPI::Stem> SoundFlipAPI::listStems(const String& sessionId)
{
    Array<Stem> stems;
    
    var response = makeRequest("/sessions/" + sessionId + "/stems");
    
    if (response.isVoid() || !response.isArray())
        return stems;
    
    for (int i = 0; i < response.size(); ++i)
    {
        stems.add(parseStem(response[i]));
    }
    
    return stems;
}

String SoundFlipAPI::getDownloadUrl(const String& stemId)
{
    var response = makeRequest("/stems/" + stemId + "/download-url");
    
    if (response.isVoid())
        return {};
    
    return response["downloadUrl"].toString();
}

bool SoundFlipAPI::deleteStem(const String& stemId)
{
    var response = makeRequest("/stems/" + stemId, "DELETE");
    return !response.isVoid();
}