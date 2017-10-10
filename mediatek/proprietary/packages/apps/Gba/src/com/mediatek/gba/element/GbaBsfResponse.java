package com.mediatek.gba.element;

import android.util.Log;

import com.mediatek.gba.header.AuthInfoHeader;
import com.mediatek.gba.header.WwwAuthHeader;

import org.apache.http.Header;
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.auth.AUTH;
import org.apache.http.auth.MalformedChallengeException;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.StringTokenizer;

/**
 * implementation for GbaBsfResponse.
 *
 * @hide
 */
public class GbaBsfResponse {
    private static final String TAG = "GbaBsfResponse";

    private static final String CONTENT_TYPE_HEADER = "Content-Type";
    private static final String BSF_XML_CONTENT_TYPE = "application/vnd.3gpp.bsf+xml";
    private static final String TMPI_INDICATOR = "3gpp-gba-tmpi";
    private static final String SERVER_HEADER_NAME = "Server";

    private int mStatusCode;
    private boolean mIsTmpiSupported;
    private String mSrverHeader;

    private WwwAuthHeader mWwwAuthHeader;

    private AuthInfoHeader mAuthInfoHeader;

    private String mXmlContent;

    protected GbaBsfResponse() {
        mIsTmpiSupported = false;
    }

    protected void parse(HttpResponse httpResponse) throws IOException {

        mStatusCode = httpResponse.getStatusLine().getStatusCode();
        Log.d(TAG, "Response Code:" + mStatusCode);

        parseServer(httpResponse);

        parseWwwAuthenticate(httpResponse);

        if (mStatusCode == 200) {
            parseAuthInfo(httpResponse);

            if (isBsfXmlContentTypePresent(httpResponse)) {
                parseXmlContent(httpResponse);
            }
        }
    }

    protected void parseServer(HttpResponse httpResponse) {

        Header serverHeader = httpResponse.getFirstHeader(SERVER_HEADER_NAME);

        if (serverHeader != null) {
            mSrverHeader = serverHeader.getValue();

            StringTokenizer st = new StringTokenizer(mSrverHeader, " ");

            while (st.hasMoreTokens()) {
                String token = st.nextToken();

                if (token.contains(TMPI_INDICATOR)) {
                    mIsTmpiSupported = true;
                    break;
                }
            }
        }
    }

    public boolean isTmpiSupported() {
        return mIsTmpiSupported;
    }

    public String getServerHeader() {
        return mSrverHeader;
    }

    public int getStatusCode() {
        return mStatusCode;
    }

    protected void parseWwwAuthenticate(HttpResponse httpResponse) throws IOException {

        Header wwwAuthHttpHeader = httpResponse
                                   .getFirstHeader(AUTH.WWW_AUTH);

        if (wwwAuthHttpHeader != null) {
            String wwwAuthHeaderValue = wwwAuthHttpHeader.getValue();

            try {
                mWwwAuthHeader = WwwAuthHeader.parse(wwwAuthHeaderValue);
            } catch (MalformedChallengeException e) {
                e.printStackTrace();
                Log.e(TAG, "Can't parse:" + wwwAuthHeaderValue);
                mWwwAuthHeader = null;
            }
        }
    }

    public WwwAuthHeader getWwwAuthenticateHeader() {
        return mWwwAuthHeader;
    }

    private boolean isBsfXmlContentTypePresent(HttpResponse httpResponse) {
        Header contentTypeHeader = httpResponse.getFirstHeader(CONTENT_TYPE_HEADER);

        if (contentTypeHeader != null) {
            String contentTypeHeaderValue = contentTypeHeader.getValue();
            Log.d(TAG, "bsf xml contenttype = " + contentTypeHeaderValue);

            if (contentTypeHeaderValue.toLowerCase().contains(BSF_XML_CONTENT_TYPE)) {
                return true;
            }
        }

        return false;
    }

    private void parseXmlContent(HttpResponse httpResponse) throws IOException {
        HttpEntity entity = httpResponse.getEntity();
        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
        entity.writeTo(byteArrayOutputStream);
        byteArrayOutputStream.close();
        mXmlContent = byteArrayOutputStream.toString();
    }

    private void parseAuthInfo(HttpResponse httpResponse) {
        Header autheInfoHttpHeader = httpResponse
                                     .getFirstHeader(AuthInfoHeader.HEADER_NAME);

        if (autheInfoHttpHeader != null) {
            String authInfoHttpHeaderValue = autheInfoHttpHeader
                                             .getValue();

            try {
                mAuthInfoHeader = AuthInfoHeader.parse(authInfoHttpHeaderValue);
            } catch (MalformedChallengeException e) {
                e.printStackTrace();
                mAuthInfoHeader = null;
            }
        }
    }

    public AuthInfoHeader getAuthenticationInfoHeader() {
        return mAuthInfoHeader;
    }

    public String getXmlContent() {
        return mXmlContent;
    }

    /**
     * Utility function to parse BSF's HTTP response.
     *
     * @param httpResponse the HTTP resposne from BSF server.
     * @return the GbaBsfResponse object.
     * @throws IOException if there is any IO error occurred.
     *
     */
    public static GbaBsfResponse parseResponse(HttpResponse httpResponse) throws IOException {
        GbaBsfResponse bsfResponse = new GbaBsfResponse();
        bsfResponse.parse(httpResponse);
        return bsfResponse;
    }

}
