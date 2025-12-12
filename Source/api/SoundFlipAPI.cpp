// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SoundFlipAPI.h"

SoundFlipAPI::SoundFlipAPI(SoundFlipAuth& authRef)
    : auth(authRef)
{
}

SoundFlipAPI::~SoundFlipAPI()
{
}

//==============================================================================
// HTTP Request Helper

var SoundFlipAPI::makeRequest(const String& endpoint, 
                               const String& method,
                               const var& body)
{
    lastError = "";
    lastStatusCode = 0;
    
    String accessToken = auth.getAccessToken();
    if (accessToken.isEmpty())
    {
        lastError = "Not authenticated";
        lastStatusCode = 401;
        return var();
    }
    
    URL url(apiBaseUrl + endpoint);
    
    String extraHeaders = "Authorization: Bearer " + accessToken + "\r\n";
    extraHeaders += "Content-Type: application/json\r\n";
    
    if (method == "POST" || method == "PATCH" || method == "PUT" || method == "DELETE")
    {
        String jsonBody = body.isVoid() ? "{}" : JSON::toString(body);
        url = url.withPOSTData(jsonBody);
    }
    
    // For methods other than GET/POST, we need to handle differently
    // JUCE's URL class primarily supports GET and POST
    // For PATCH/DELETE, we'll use POST with method override or handle via headers
    
    std::unique_ptr<InputStream> stream;
    
    if (method == "GET")
    {
        auto options = URL::InputStreamOptions(URL::ParameterHandling::inAddress)
            .withExtraHeaders(extraHeaders)
            .withConnectionTimeoutMs(30000);
        
        stream = url.createInputStream(options);
    }
    else
    {
        // Add method override header for non-GET/POST
        if (method == "PATCH" || method == "DELETE" || method == "PUT")
        {
            extraHeaders += "X-HTTP-Method-Override: " + method + "\r\n";
        }
        
        auto options = URL::InputStreamOptions(URL::ParameterHandling::inPostData)
            .withExtraHeaders(extraHeaders)
            .withHttpRequestCmd(method)
            .withConnectionTimeoutMs(30000);
        
        stream = url.createInputStream(options);
    }
    
    if (stream == nullptr)
    {
        lastError = "Failed to connect to server";
        lastStatusCode = 0;
        return var();
    }
    
    String response = stream->readEntireStreamAsString();
    
    // Try to get status code (JUCE doesn't expose this directly, but we can infer from response)
    var result;
    Result parseResult = JSON::parse(response, result);
    
    if (parseResult.failed())
    {
        lastError = "Invalid JSON response";
        lastStatusCode = 500;
        return var();
    }
    
    // Check for error in response
    if (result.hasProperty("statusCode") && (int)result["statusCode"] >= 400)
    {
        lastStatusCode = (int)result["statusCode"];
        lastError = result.hasProperty("message") ? result["message"].toString() : "Request failed";
        return var();
    }
    
    lastStatusCode = 200;
    return result;
}

//==============================================================================
// Collab Session Management

SoundFlipAPI::CollabSession SoundFlipAPI::createCollabSession(const String& name)
{
    var body;
    DynamicObject::Ptr obj = new DynamicObject();
    
    if (name.isNotEmpty())
        obj->setProperty("name", name);
    
    body = var(obj.get());
    
    var response = makeRequest("/api/collab-sessions", "POST", body);
    
    if (response.isVoid())
        return CollabSession();
    
    return parseCollabSession(response);
}

SoundFlipAPI::CollabSession SoundFlipAPI::getCollabSession(const String& sessionId)
{
    var response = makeRequest("/api/collab-sessions/" + sessionId);
    
    if (response.isVoid())
        return CollabSession();
    
    return parseCollabSession(response);
}

SoundFlipAPI::CollabSession SoundFlipAPI::getCollabSessionByInviteCode(const String& inviteCode)
{
    var response = makeRequest("/api/collab-sessions/invite/" + inviteCode);
    
    if (response.isVoid())
        return CollabSession();
    
    return parseCollabSession(response);
}

SoundFlipAPI::CollabSession SoundFlipAPI::joinCollabSession(const String& sessionIdOrInviteCode)
{
    var response = makeRequest("/api/collab-sessions/" + sessionIdOrInviteCode + "/join", "POST");
    
    if (response.isVoid())
        return CollabSession();
    
    return parseCollabSession(response);
}

bool SoundFlipAPI::leaveCollabSession(const String& sessionId)
{
    var response = makeRequest("/api/collab-sessions/" + sessionId + "/leave", "POST");
    
    if (response.isVoid())
        return false;
    
    return response.hasProperty("success") && (bool)response["success"];
}

SoundFlipAPI::CollabSession SoundFlipAPI::updateCollabSession(const String& sessionId, 
                                                              const String& name, 
                                                              const String& status)
{
    var body;
    DynamicObject::Ptr obj = new DynamicObject();
    
    if (name.isNotEmpty())
        obj->setProperty("name", name);
    
    if (status.isNotEmpty())
        obj->setProperty("status", status);
    
    body = var(obj.get());
    
    var response = makeRequest("/api/collab-sessions/" + sessionId, "PATCH", body);
    
    if (response.isVoid())
        return CollabSession();
    
    return parseCollabSession(response);
}

Array<SoundFlipAPI::CollabSession> SoundFlipAPI::listCollabSessions(const String& status, 
                                                                     int limit, 
                                                                     int offset)
{
    String endpoint = "/api/collab-sessions?limit=" + String(limit) + "&offset=" + String(offset);
    
    if (status.isNotEmpty())
        endpoint += "&status=" + status;
    
    var response = makeRequest(endpoint);
    
    Array<CollabSession> sessions;
    
    if (response.isVoid())
        return sessions;
    
    if (response.hasProperty("sessions") && response["sessions"].isArray())
    {
        auto* sessionsArray = response["sessions"].getArray();
        for (const auto& sessionJson : *sessionsArray)
        {
            sessions.add(parseCollabSession(sessionJson));
        }
    }
    
    return sessions;
}

//==============================================================================
// Stem Management

SoundFlipAPI::UploadUrlResponse SoundFlipAPI::requestStemUploadUrl(const String& sessionId, 
                                                                    const String& filename, 
                                                                    const String& contentType,
                                                                    int64 sizeBytes)
{
    var body;
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty("filename", filename);
    obj->setProperty("contentType", contentType);
    obj->setProperty("sizeBytes", sizeBytes);
    body = var(obj.get());
    
    var response = makeRequest("/api/collab-sessions/" + sessionId + "/stems/upload-url", "POST", body);
    
    if (response.isVoid())
        return UploadUrlResponse();
    
    return parseUploadUrlResponse(response);
}

SoundFlipAPI::Stem SoundFlipAPI::completeStemUpload(const String& sessionId, 
                                                     const String& stemId, 
                                                     int durationSeconds)
{
    var body;
    DynamicObject::Ptr obj = new DynamicObject();
    
    if (durationSeconds > 0)
        obj->setProperty("durationSeconds", durationSeconds);
    
    body = var(obj.get());
    
    var response = makeRequest("/api/collab-sessions/" + sessionId + "/stems/" + stemId + "/complete", 
                               "POST", body);
    
    if (response.isVoid())
        return Stem();
    
    // Response has { success: bool, stem: {...} }
    if (response.hasProperty("stem"))
        return parseStem(response["stem"]);
    
    return Stem();
}

Array<SoundFlipAPI::Stem> SoundFlipAPI::listSessionStems(const String& sessionId)
{
    var response = makeRequest("/api/collab-sessions/" + sessionId + "/stems");
    
    Array<Stem> stems;
    
    if (response.isVoid())
        return stems;
    
    if (response.hasProperty("stems") && response["stems"].isArray())
    {
        auto* stemsArray = response["stems"].getArray();
        for (const auto& stemJson : *stemsArray)
        {
            stems.add(parseStem(stemJson));
        }
    }
    
    return stems;
}

bool SoundFlipAPI::deleteStem(const String& sessionId, const String& stemId)
{
    var response = makeRequest("/api/collab-sessions/" + sessionId + "/stems/" + stemId, "DELETE");
    
    if (response.isVoid())
        return false;
    
    return response.hasProperty("success") && (bool)response["success"];
}

//==============================================================================
// S3 Upload Helper

bool SoundFlipAPI::uploadFileToS3(const String& presignedUrl, 
                                   const File& file, 
                                   const String& contentType)
{
    if (!file.existsAsFile())
    {
        lastError = "File does not exist";
        return false;
    }
    
    // Read file into memory
    MemoryBlock fileData;
    if (!file.loadFileAsData(fileData))
    {
        lastError = "Failed to read file";
        return false;
    }
    
    // Create URL and upload
    URL url(presignedUrl);
    
    String extraHeaders = "Content-Type: " + contentType + "\r\n";
    extraHeaders += "Content-Length: " + String(fileData.getSize()) + "\r\n";
    
    // Set the file data as POST data
    url = url.withPOSTData(fileData);
    
    auto options = URL::InputStreamOptions(URL::ParameterHandling::inPostData)
        .withExtraHeaders(extraHeaders)
        .withConnectionTimeoutMs(300000)  // 5 min timeout for large files
        .withHttpRequestCmd("PUT");
    
    auto stream = url.createInputStream(options);
    
    if (stream == nullptr)
    {
        lastError = "Failed to upload to S3";
        return false;
    }
    
    // Read response (S3 returns empty body on success)
    String response = stream->readEntireStreamAsString();
    
    // S3 PUT returns 200 on success with empty body
    // If there's an error, it would contain XML error message
    if (response.contains("<Error>"))
    {
        lastError = "S3 upload error: " + response;
        return false;
    }
    
    return true;
}

//==============================================================================
// JSON Parsing Helpers

SoundFlipAPI::CollabSession SoundFlipAPI::parseCollabSession(const var& json)
{
    CollabSession session;
    
    session.id = json.getProperty("id", "").toString();
    session.inviteCode = json.getProperty("inviteCode", "").toString();
    session.name = json.getProperty("name", "").toString();
    session.status = json.getProperty("status", "").toString();
    session.inviteUrl = json.getProperty("inviteUrl", "").toString();
    session.stemCount = (int)json.getProperty("stemCount", 0);
    session.durationSeconds = (int)json.getProperty("durationSeconds", 0);
    
    // Parse connection info if present
    if (json.hasProperty("connection"))
    {
        session.connection = parseConnectionInfo(json["connection"]);
    }
    
    // Parse creator info
    if (json.hasProperty("createdBy"))
    {
        var creator = json["createdBy"];
        session.createdById = creator.getProperty("id", "").toString();
        session.createdByUsername = creator.getProperty("username", "").toString();
        session.createdByAvatar = creator.getProperty("avatar", "").toString();
    }
    
    // Parse participants
    if (json.hasProperty("participants") && json["participants"].isArray())
    {
        auto* participantsArray = json["participants"].getArray();
        for (const auto& p : *participantsArray)
        {
            session.participants.add(parseParticipant(p));
        }
    }
    
    // Parse timestamps
    if (json.hasProperty("createdAt"))
    {
        // Parse ISO date string to timestamp
        String dateStr = json["createdAt"].toString();
        Time t = Time::fromISO8601(dateStr);
        session.createdAt = t.toMilliseconds();
    }
    
    if (json.hasProperty("endedAt") && !json["endedAt"].isVoid())
    {
        String dateStr = json["endedAt"].toString();
        Time t = Time::fromISO8601(dateStr);
        session.endedAt = t.toMilliseconds();
    }
    
    return session;
}

SoundFlipAPI::Participant SoundFlipAPI::parseParticipant(const var& json)
{
    Participant p;
    
    p.userId = json.getProperty("userId", "").toString();
    p.username = json.getProperty("username", "").toString();
    p.avatar = json.getProperty("avatar", "").toString();
    
    if (json.hasProperty("joinedAt"))
    {
        String dateStr = json["joinedAt"].toString();
        Time t = Time::fromISO8601(dateStr);
        p.joinedAt = t.toMilliseconds();
    }
    
    if (json.hasProperty("leftAt") && !json["leftAt"].isVoid())
    {
        String dateStr = json["leftAt"].toString();
        Time t = Time::fromISO8601(dateStr);
        p.leftAt = t.toMilliseconds();
    }
    
    return p;
}

SoundFlipAPI::ConnectionInfo SoundFlipAPI::parseConnectionInfo(const var& json)
{
    ConnectionInfo info;
    
    info.server = json.getProperty("server", "").toString();
    info.port = (int)json.getProperty("port", 10999);
    info.group = json.getProperty("group", "").toString();
    info.password = json.getProperty("password", "").toString();
    
    return info;
}

SoundFlipAPI::Stem SoundFlipAPI::parseStem(const var& json)
{
    Stem stem;
    
    stem.id = json.getProperty("id", "").toString();
    stem.filename = json.getProperty("filename", "").toString();
    stem.downloadUrl = json.getProperty("downloadUrl", "").toString();
    stem.sizeBytes = (int64)json.getProperty("sizeBytes", 0);
    stem.durationSeconds = (int)json.getProperty("durationSeconds", 0);
    
    // Parse uploader info
    if (json.hasProperty("uploadedBy"))
    {
        var uploader = json["uploadedBy"];
        stem.uploadedById = uploader.getProperty("id", "").toString();
        stem.uploadedByUsername = uploader.getProperty("username", "").toString();
        stem.uploadedByAvatar = uploader.getProperty("avatar", "").toString();
    }
    
    if (json.hasProperty("createdAt"))
    {
        String dateStr = json["createdAt"].toString();
        Time t = Time::fromISO8601(dateStr);
        stem.createdAt = t.toMilliseconds();
    }
    
    return stem;
}

SoundFlipAPI::UploadUrlResponse SoundFlipAPI::parseUploadUrlResponse(const var& json)
{
    UploadUrlResponse response;
    
    response.uploadUrl = json.getProperty("uploadUrl", "").toString();
    response.stemId = json.getProperty("stemId", "").toString();
    response.s3Key = json.getProperty("s3Key", "").toString();
    response.expiresIn = (int)json.getProperty("expiresIn", 3600);
    
    return response;
}