#if !defined(RESPONSE_HEADER_H)
#define RESPONSE_HEADER_H

#include "utils.h"
#include <map>
#include <string>
#include <sys/stat.h>

using std::map;
using std::string;
using std::to_string;

class response_header {
public:
    static map<int, string> code2phrase;
    static string STR_VERSION_HTTP_1_1;

    static const int CODE_INTERNAL_SERVER_ERROR;
    static const int CODE_NOT_FOUND;
    static const int CODE_OK;

    static map<string, string> mp_file_suffix_2_content_type;

    string version;
    int status_code;
    string phrase;
    map<string, string> mp_gene_headers;

    response_header() {}
    ~response_header() {}

    // header with content-type:html, content-length:size of file of [path]
    static int htmlHeader(string path_file, response_header &header, int status_code = 200) {
        int ret = 0;
        struct stat file_info;
        ret = stat(path_file.c_str(), &file_info);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to stat file: ", path_file, " failed"}, true);
            return -1;
        }
        header.version = STR_VERSION_HTTP_1_1;
        header.status_code = status_code;
        header.phrase = code2phrase[status_code];
        header.mp_gene_headers["Content-Type"] = "text/html;charset=UTF-8";
        header.mp_gene_headers["Content-Length"] = to_string(file_info.st_size);
        return 0;
    }
    static int strHeader(const string &data, response_header &header, int status_code = 200) {
        header.version = STR_VERSION_HTTP_1_1;
        header.status_code = status_code;
        header.phrase = code2phrase[status_code];
        header.mp_gene_headers["Content-Type"] = "text/plain;charset=UTF-8";
        header.mp_gene_headers["Content-Length"] = to_string(data.size());
        return 0;
    }
    static int fileHeader(const string path_file, response_header &header, int status_code = 200) {
        int ret = 0;
        struct stat file_info;
        ret = stat(path_file.c_str(), &file_info);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to stat file: ", path_file, " failed"}, true);
            return -1;
        }
        header.version = STR_VERSION_HTTP_1_1;
        header.status_code = status_code;
        header.phrase = code2phrase[status_code];
        header.mp_gene_headers["Content-Type"] = "text/plain;charset=UTF-8";
        header.mp_gene_headers["Content-Length"] = to_string(file_info.st_size);
        // set content-type according to suffix
        size_t last_pos_of_dot = path_file.find_last_of('.');
        if (last_pos_of_dot != string::npos) {
            const string file_type = path_file.substr(last_pos_of_dot);
            if (mp_file_suffix_2_content_type.find(file_type) != mp_file_suffix_2_content_type.end())
                header.mp_gene_headers["Content-Type"] = mp_file_suffix_2_content_type[file_type];
        }
        return 0;
    }
    string toString() {
        stringstream ss;
        const char sp = ' ';
        const string crlf = "\r\n";
        const char colon = ':';
        ss << version << sp << status_code << sp << phrase << crlf;
        for (auto &&p : mp_gene_headers)
            ss << p.first << colon << p.second << crlf;
        ss << crlf;
        return ss.str();
    }
};

const int response_header::CODE_OK = 200;
const int response_header::CODE_NOT_FOUND = 404;
const int response_header::CODE_INTERNAL_SERVER_ERROR = 500;

string response_header::STR_VERSION_HTTP_1_1 = "HTTP/1.1";
map<int, string> response_header::code2phrase;
map<string, string> response_header::mp_file_suffix_2_content_type = {
    {".tif", "image/tiff"},
    {".asf", "video/x-ms-asf"},
    {".asp", "text/asp"},
    {".asx", "video/x-ms-asf"},
    {".au", "audio/basic"},
    {".avi", "video/avi"},
    {".awf", "application/vnd.adobe.workflow"},
    {".biz", "text/xml"},
    {".bmp", "application/x-bmp"},
    {".bot", "application/x-bot"},
    {".cdr", "application/x-cdr"},
    {".cel", "application/x-cel"},
    {".cer", "application/x-x509-ca-cert"},
    {".cg4", "application/x-g4"},
    {".cgm", "application/x-cgm"},
    {".cit", "application/x-cit"},
    {".class", "java/*"},
    {".cml", "text/xml"},
    {".cmp", "application/x-cmp"},
    {".cmx", "application/x-cmx"},
    {".cot", "application/x-cot"},
    {".crl", "application/pkix-crl"},
    {".crt", "application/x-x509-ca-cert"},
    {".csi", "application/x-csi"},
    {".css", "text/css"},
    {".cut", "application/x-cut"},
    {".dbf", "application/x-dbf"},
    {".dbm", "application/x-dbm"},
    {".dbx", "application/x-dbx"},
    {".dcd", "text/xml"},
    {".dcx", "application/x-dcx"},
    {".der", "application/x-x509-ca-cert"},
    {".dgn", "application/x-dgn"},
    {".dib", "application/x-dib"},
    {".dll", "application/x-msdownload"},
    {".doc", "application/msword"},
    {".dot", "application/msword"},
    {".drw", "application/x-drw"},
    {".dtd", "text/xml"},
    {".dwf", "Model/vnd.dwf"},
    {".dwf", "application/x-dwf"},
    {".dwg", "application/x-dwg"},
    {".dxb", "application/x-dxb"},
    {".dxf", "application/x-dxf"},
    {".edn", "application/vnd.adobe.edn"},
    {".emf", "application/x-emf"},
    {".eml", "message/rfc822"},
    {".ent", "text/xml"},
    {".epi", "application/x-epi"},
    {".eps", "application/x-ps"},
    {".eps", "application/postscript"},
    {".etd", "application/x-ebx"},
    {".exe", "application/x-msdownload"},
    {".fax", "image/fax"},
    {".fdf", "application/vnd.fdf"},
    {".fif", "application/fractals"},
    {".fo", "text/xml"},
    {".frm", "application/x-frm"},
    {".g4", "application/x-g4"},
    {".gbr", "application/x-gbr"},
    {".", "application/x-"},
    {".gif", "image/gif"},
    {".gl2", "application/x-gl2"},
    {".gp4", "application/x-gp4"},
    {".hgl", "application/x-hgl"},
    {".hmr", "application/x-hmr"},
    {".hpg", "application/x-hpgl"},
    {".hpl", "application/x-hpl"},
    {".hqx", "application/mac-binhex40"},
    {".hrf", "application/x-hrf"},
    {".hta", "application/hta"},
    {".htc", "text/x-component"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".htt", "text/webviewhtml"},
    {".htx", "text/html"},
    {".icb", "application/x-icb"},
    {".ico", "image/x-icon"},
    {".ico", "application/x-ico"},
    {".iff", "application/x-iff"},
    {".ig4", "application/x-g4"},
    {".igs", "application/x-igs"},
    {".iii", "application/x-iphone"},
    {".img", "application/x-img"},
    {".ins", "application/x-internet-signup"},
    {".isp", "application/x-internet-signup"},
    {".IVF", "video/x-ivf"},
    {".java", "java/*"},
    {".jfif", "image/jpeg"},
    {".jpe", "image/jpeg"},
    {".jpe", "application/x-jpe"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".jpg", "application/x-jpg"},
    {".js", "application/x-javascript"},
    {".jsp", "text/html"},
    {".la1", "audio/x-liquid-file"},
    {".lar", "application/x-laplayer-reg"},
    {".latex", "application/x-latex"},
    {".lavs", "audio/x-liquid-secure"},
    {".lbm", "application/x-lbm"},
    {".lmsff", "audio/x-la-lms"},
    {".ls", "application/x-javascript"},
    {".ltr", "application/x-ltr"},
    {".m1v", "video/x-mpeg"},
    {".m2v", "video/x-mpeg"},
    {".m3u", "audio/mpegurl"},
    {".m4e", "video/mpeg4"},
    {".mac", "application/x-mac"},
    {".man", "application/x-troff-man"},
    {".math", "text/xml"},
    {".mdb", "application/msaccess"},
    {".mdb", "application/x-mdb"},
    {".mfp", "application/x-shockwave-flash"},
    {".mht", "message/rfc822"},
    {".mhtml", "message/rfc822"},
    {".mp1", "audio/mp1"},
    {".mp2", "audio/mp2"},
    {".mp2v", "video/mpeg"},
    {".mp3", "audio/mp3"},
    {".mp4", "video/mpeg4"},
    {".mpa", "video/x-mpg"},
    {".mpd", "application/vnd.ms-project"},
    {".mpe", "video/x-mpeg"},
    {".mpeg", "video/mpg"},
    {".mpg", "video/mpg"},
    {".mpga", "audio/rn-mpeg"},
    {".mpp", "application/vnd.ms-project"},
    {".mps", "video/x-mpeg"},
    {".mpt", "application/vnd.ms-project"},
    {".mpv", "video/mpg"},
    {".mpv2", "video/mpeg"},
    {".mpw", "application/vnd.ms-project"},
    {".mpx", "application/vnd.ms-project"},
    {".mtx", "text/xml"},
    {".mxp", "application/x-mmxp"},
    {".net", "image/pnetvue"},
    {".nrf", "application/x-nrf"},
    {".nws", "message/rfc822"},
    {".odc", "text/x-ms-odc"},
    {".out", "application/x-out"},
    {".pdf", "application/pdf"},
    {".pdf", "application/pdf"},
    {".plt", "application/x-plt"},
    {".png", "image/png"},
    {".png", "application/x-png"},
    {".pot", "application/vnd.ms-powerpoint"},
    {".ppa", "application/vnd.ms-powerpoint"},
    {".ppm", "application/x-ppm"},
    {".pps", "application/vnd.ms-powerpoint"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".ppt", "application/x-ppt"},
    {".pr", "application/x-pr"},
    {".prf", "application/pics-rules"},
    {".prn", "application/x-prn"},
    {".prt", "application/x-prt"},
    {".ps", "application/x-ps"},
    {".ps", "application/postscript"},
    {".ptn", "application/x-ptn"},
    {".rec", "application/vnd.rn-recording"},
    {".red", "application/x-red"},
    {".rgb", "application/x-rgb"},
    {".rm", "application/vnd.rn-realmedia"},
    {".rmf", "application/vnd.adobe.rmf"},
    {".rmi", "audio/mid"},
    {".rmj", "application/vnd.rn-realsystem-rmj"},
    {".rmm", "audio/x-pn-realaudio"},
    {".rmp", "application/vnd.rn-rn_music_package"},
    {".rms", "application/vnd.rn-realmedia-secure"},
    {".rmvb", "application/vnd.rn-realmedia-vbr"},
    {".rmx", "application/vnd.rn-realsystem-rmx"},
    {".rnx", "application/vnd.rn-realplayer"},
    {".rp", "image/vnd.rn-realpix"},
    {".rpm", "audio/x-pn-realaudio-plugin"},
    {".rsml", "application/vnd.rn-rsml"},
    {".rt", "text/vnd.rn-realtext"},
    {".rtf", "application/msword"},
    {".rtf", "application/x-rtf"},
    {".rv", "video/vnd.rn-realvideo"},
    {".sam", "application/x-sam"},
    {".sat", "application/x-sat"},
    {".sdp", "application/sdp"},
    {".spc", "application/x-pkcs7-certificates"},
    {".spl", "application/futuresplash"},
    {".spp", "text/xml"},
    {".ssm", "application/streamingmedia"},
    {".sst", "application/vnd.ms-pki.certstore"},
    {".stl", "application/vnd.ms-pki.stl"},
    {".stm", "text/html"},
    {".sty", "application/x-sty"},
    {".svg", "text/xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tdf", "application/x-tdf"},
    {".tg4", "application/x-tg4"},
    {".tga", "application/x-tga"},
    {".tif", "image/tiff"},
    {".tif", "application/x-tif"},
    {".tiff", "image/tiff"},
    {".tld", "text/xml"},
    {".top", "drawing/x-top"},
    {".torrent", "application/x-bittorrent"},
    {".tsd", "text/xml"},
    {".txt", "text/plain"},
    {".uin", "application/x-icq"},
    {".uls", "text/iuls"},
    {".vcf", "text/x-vcard"},
    {".vda", "application/x-vda"},
    {".vdx", "application/vnd.visio"},
    {".vml", "text/xml"},
    {".vpg", "application/x-vpeg005"},
    {".vsd", "application/vnd.visio"},
    {".vsd", "application/x-vsd"},
    {".vss", "application/vnd.visio"},
    {".vst", "application/vnd.visio"},
    {".vst", "application/x-vst"},
    {".vsw", "application/vnd.visio"},
    {".vsx", "application/vnd.visio"},
    {".vtx", "application/vnd.visio"},
    {".vxml", "text/xml"},
    {".wav", "audio/wav"},
    {".wax", "audio/x-ms-wax"},
    {".wb1", "application/x-wb1"},
    {".wb2", "application/x-wb2"},
    {".wb3", "application/x-wb3"},
    {".wbmp", "image/vnd.wap.wbmp"},
    {".wiz", "application/msword"},
    {".wk3", "application/x-wk3"},
    {".wk4", "application/x-wk4"},
    {".wkq", "application/x-wkq"},
    {".wks", "application/x-wks"},
    {".wm", "video/x-ms-wm"},
    {".wma", "audio/x-ms-wma"},
    {".wmd", "application/x-ms-wmd"},
    {".wmf", "application/x-wmf"},
    {".wml", "text/vnd.wap.wml"},
    {".wmv", "video/x-ms-wmv"},
    {".wmx", "video/x-ms-wmx"},
    {".wmz", "application/x-ms-wmz"},
    {".wp6", "application/x-wp6"},
    {".wpd", "application/x-wpd"},
    {".wpg", "application/x-wpg"},
    {".wpl", "application/vnd.ms-wpl"},
    {".wq1", "application/x-wq1"},
    {".wr1", "application/x-wr1"},
    {".wri", "application/x-wri"},
    {".wrk", "application/x-wrk"},
    {".ws", "application/x-ws"},
    {".ws2", "application/x-ws"},
    {".wsc", "text/scriptlet"},
    {".wsdl", "text/xml"},
    {".wvx", "video/x-ms-wvx"},
    {".xdp", "application/vnd.adobe.xdp"},
    {".xdr", "text/xml"},
    {".xfd", "application/vnd.adobe.xfd"},
    {".xfdf", "application/vnd.adobe.xfdf"},
    {".xhtml", "text/html"},
    {".xls", "application/vnd.ms-excel"},
    {".xls", "application/x-xls"},
    {".xlw", "application/x-xlw"},
    {".xml", "text/xml"},
    {".xpl", "audio/scpls"},
    {".xq", "text/xml"},
    {".xql", "text/xml"},
    {".xquery", "text/xml"},
    {".ipa", "application/vnd.iphone"},
    {".apk", "application/vnd.android.package-archive"},
};
#endif // RESPONSE_HEADER_H
