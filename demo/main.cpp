#include "contract_compare.h"
#include <iostream>
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"
#include "diff.h"
#include <dirent.h>
#include <vector>
#include <locale>
#include <codecvt>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <experimental/filesystem>
#include <regex>
#include "nlohmann/json.hpp"
#include "httplib/httplib.h"
#include "yaml-cpp/yaml.h"
#include "md5.h"
#include "base64.h"
//#include "pdf2img.h"
//#include "word2img.h"
#include "xie_filesystem.h"
#include "str_utils.h"
#include "xie_debug.h"

using namespace std;

namespace fs = std::experimental::filesystem;

// 图片目录
static std::string g_scan_path;

// 文件名匹配
static std::string g_file_regex;


std::string g_root;


struct PostParam 
{
    std::string host;
    int port;
    std::string path;

    bool is_valid()
    {
        return (host.size() && port > 0 && path.size());
    }
};

static PostParam g_ocr_param;


void loadConfig(const std::string &filename)
{
    YAML::Node config = YAML::LoadFile(filename);
    g_ocr_param.host = config["ocr"]["host"].as<std::string>();
    g_ocr_param.port = config["ocr"]["port"].as<int>();
    g_ocr_param.path = config["ocr"]["path"].as<std::string>();
    //读取指定路径的 YAML 配置文件，并将配置信息解析成一个 YAML::Node ，读取指定路径的 YAML 配置文件，并将配置信息解析成一个 YAML::Node 
    g_scan_path = config["scan_path"].as<std::string>();

    if (config["file_regex"])//判断配置文件中是否包含 "file_regex" 键
    {
        g_file_regex = config["file_regex"].as<std::string>();
    }
}


string global_file_name;

string recogSlices(void* p, cv::Mat& image, string type,
                   const vector<int>& slice_vec,
                   map<string, string> map_recog_params) 
{
#if 1
  bool need_red = false;
  if (map_recog_params.find("color_channel") != map_recog_params.end() &
      map_recog_params["color_channel"] == "red") {
    need_red = true;
  }
  cv::Mat painter = image.clone();
  int width = int(painter.cols);
  int height = int(painter.rows);

  vector<vector<int> > slices;

  for (int i = 0; i < slice_vec.size(); i += 4) {
    vector<int> tmp_sl = {slice_vec[i + 0], slice_vec[i + 1], slice_vec[i + 2],
                          slice_vec[i + 3]};
    slices.push_back(tmp_sl);
    cv::rectangle(painter, cv::Point(slice_vec[i + 0], slice_vec[i + 1]),
                  cv::Point(slice_vec[i + 2], slice_vec[i + 3]),
                  cv::Scalar(255), 2, 2);
  }

  cv::Mat tmp = painter(cv::Rect(cv::Point(0, 0), cv::Point(width, height)));

  if (need_red) {
    cv::Mat rgbChannels[3];
    cv::split(tmp, rgbChannels);
    cv::Mat blue = rgbChannels[2];
    cv::cvtColor(blue, tmp, cv::COLOR_GRAY2BGR);
  }

  // cv::namedWindow("slices", CV_WINDOW_NORMAL);
  // cv::imshow("slices", tmp);
  // cv::waitKey(0);
  // cv::imwrite("./tmp.jpg", tmp);

  vector<uchar> buf;
  cv::imencode(".jpg", tmp, buf);
  string imgStr = xwb::base64_encode(buf.data(), buf.size());
 

  nlohmann::json slisJson;
  slisJson["image"] = imgStr;
  slisJson["slices"] = slices;
  string body = slisJson.dump();
  std::cout << "httplib::Client" << std::endl;
// cout << body << endl;

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  httplib::SSLClient cli("ai.intsig.net", 443);
#else
  httplib::Client cli("sh-imgs-sandbox.intsig.net");
  // httplib::Client cli("192.168.16.117", 4248);
#endif

  std::cout << " cli "
            << "body length" << body.length() << std::endl;

  try {
    // auto res = cli.Post("/icr/ctpn_recognize_slices_no_detect", body,
    // "json");
    auto res = cli.Post("/1026/icr3/ctpn_recognize_slices", body, "json");
    std::cout << " recog resp " << res->status << std::endl;
    if (res->status != 200) {
      return "-1";
    }

    std::string json = res->body;
    return json;

  } catch (exception& e) {

    cout << e.what() << endl;

  }

#endif

  return "-1";

}

std::string api_file_type;

//接受三个参数：unsigned char *buf 表示二进制数据的指针，int size 表示二进制数据的大小，PostParam& param 表示存储请求参数的结构体。
std::string recognizeBufByAPIFormData(unsigned char *buf, int size, PostParam& param)
{
    //这行代码将接收到的二进制数据转换为字符串格式，并将其保存在名为 body 的字符串变量中。这个字符串将被用作 API 请求的主体。
    std::string body((char *)buf, size);
    //根据是否定义了 CPPHTTPLIB_OPENSSL_SUPPORT 宏来选择不同的代码
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
//根据条件编译选择不同的构造函数来创建一个 HTTP 客户端对象 cli，用于与目标 API 服务器建立连接。
    httplib::SSLClient cli(param.host.c_str(), param.port);
#else
    httplib::Client cli(param.host.c_str(), param.port);
#endif
//这两行代码创建了一个空的多部分表单数据项的列表 formdatas，以及一个单个表单数据项 formdata。
    httplib::MultipartFormDataItems formdatas;
    httplib::MultipartFormData formdata;
    //设置了表单数据项 formdata 的名称为 "file"，内容为之前转换的字符串 body
    //以及文件名为 api_file_type（根据之前的逻辑判断确定）。然后将该表单数据项添加到表单数据列表 formdatas 中。
    formdata.name = "file";
    formdata.content = body;
    formdata.filename = api_file_type;
    formdatas.push_back(formdata);
    //接下来，这段代码设置了一系列其他表单数据项的名称和内容，并将它们添加到表单数据列表 formdatas 中。
    {
        httplib::MultipartFormData fd;
        fd.name = "engine";
        fd.content = "table";
        formdatas.push_back(fd);
    }

    {
         httplib::MultipartFormData fd;
         fd.name = "merge_same_row";
         fd.content = "true";
         formdatas.push_back(fd);
    }

    {
         httplib::MultipartFormData fd;
         fd.name = "apply_stamp";
         fd.content = "1";
         formdatas.push_back(fd);
    }
    //     {
    //      httplib::MultipartFormData fd;
    //      fd.name = "remove_stamp";
    //      fd.content = "true";
    //      formdatas.push_back(fd);
    // }
    {
         httplib::MultipartFormData fd;
         fd.name = "use_pdf_parser";
         fd.content = "true";
         formdatas.push_back(fd);
    }
    {
         httplib::MultipartFormData fd;
         fd.name = "remove_footnote";
         fd.content = "false";
         formdatas.push_back(fd);
    }
{
         httplib::MultipartFormData fd;
         fd.name = "remove_annot";
         fd.content = "false";
         formdatas.push_back(fd);
    }
    {
         httplib::MultipartFormData fd;
         fd.name = "remove_edge";
         fd.content = "false";
         formdatas.push_back(fd);
    }
    {
         httplib::MultipartFormData fd;
         fd.name = "return_image";
         fd.content = "true";
         formdatas.push_back(fd);
    }
    // {
    //      httplib::MultipartFormData fd;
    //      fd.name = "image_scale";
    //      fd.content = "2";
    //      formdatas.push_back(fd);
    // }
    std::cout << " cli "
              << "body length" << body.length() << std::endl;
    try
    {
        //在 try 块中执行 HTTP POST 请求，并尝试获取响应结果。如果请求发生异常，将会进入 catch 块，并输出异常信息到控制台。
        auto res = cli.Post(param.path.c_str(), formdatas);
        std::string json = res->body;
        return json;
    }
    catch (exception &e)
    {
        cout << e.what() << endl;
    }
    return "";
}


std::string recognizeBufByAPI(unsigned char *buf, int size, PostParam& param)
{
    //调用另一个函数 recognizeBufByAPIFormData，将接收到的参数传递给它，并返回该函数的返回值。它的作用是处理 API 请求的表单数据。
    return recognizeBufByAPIFormData(buf, size, param);

    //这行代码将接收到的二进制数据转换为字符串格式，并将其保存在名为 body 的字符串变量中。这个字符串将被用作 API 请求的主体。
    std::string body((char *)buf, size);
    //根据是否定义了 CPPHTTPLIB_OPENSSL_SUPPORT 宏来选择不同的代码执行路径。
    //在这个例子中，如果定义了该宏，将会执行 ifdef 块中的代码，否则将执行 else 块中的代码。

    //这两行代码根据条件编译选择不同的构造函数来创建一个 HTTP 客户端对象 cli，用于与目标 API 服务器建立连接。
    //如果定义了 CPPHTTPLIB_OPENSSL_SUPPORT 宏，将创建一个支持 SSL 的客户端，否则将创建一个普通的客户端。
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(param.host.c_str(), param.port);
#else
    httplib::Client cli(param.host.c_str(), param.port);
#endif

    std::cout << " cli "
              << "body length" << body.length() << std::endl;
    try
    {
        //这行代码执行 HTTP POST 请求，将请求主体 body 发送给指定的 API 地址 param.path.c_str()
        //并设置请求的 Content-Type 为 "image/*"。请求的响应结果将保存在名为 res 的变量中。
        auto res = cli.Post(param.path.c_str(), body, "image/*");
        //这行代码将从 HTTP 响应中获取到的 JSON 数据保存在名为 json 的字符串变量中。
        std::string json = res->body;
        return json;
    }
    catch (exception &e)
    {
        cout << e.what() << endl;
    }
}

std::string recognizeBuf(unsigned char *buf, int size, std::string raw_dir)
{
    //调用 xwb::MD5Digest 函数，对传入的二进制数据进行 MD5 哈希运算，并将结果保存在名为 md5_str 的字符串变量中。
    std::string md5_str = xwb::MD5Digest(buf, size);
    //构造了一个文件名，表示识别结果的文件路径。
    std::string filename = raw_dir + "/" + md5_str + ".json";
    cout << filename << endl;
    std::string ocr_result;
    //检查识别结果文件是否已存在。如果文件已存在，说明之前对相同的数据进行过识别，可以直接读取已有的识别结果。
    //如果文件不存在，则说明还没有对该数据进行过识别，需要调用 API 进行识别。
    if(fs::exists(filename))
    {
        //这几行代码用于读取已有的识别结果。首先，打开识别结果文件，然后使用 nlohmann::json 解析文件中的 JSON 数据，并将结果保存在 j 变量中。
        //接着，将 j 变量的 JSON 数据转换为字符串格式，并将结果保存在 ocr_result 变量中。
        std::string line_str;
        std::ifstream ifs(filename, std::ifstream::in);
        nlohmann::json j;
        ifs >> j;
        ocr_result = j.dump();
        ifs.close();
    }
    else
    {
        //调用另一个函数 recognizeBufByAPI，用于对二进制数据进行识别，并将识别结果保存在 ocr_result 变量中。
        //函数调用时，传递了二进制数据、数据大小以及一些参数 g_ocr_param。
        ocr_result = recognizeBufByAPI(buf, size, g_ocr_param);
        //这几行代码将识别结果保存到文件中。首先，打开识别结果文件，然后将识别结果写入文件。最后，关闭文件。
        ofstream ofs(filename, std::ofstream::out);
        ofs << ocr_result;
        ofs.close();
    }
    return ocr_result;
}

std::string recognizeMat(const cv::Mat& image, std::string raw_dir)
{
    vector<uchar> vbuf;
    cv::imencode(".jpg", image, vbuf);
    return recognizeBuf(vbuf.data(), vbuf.size(), raw_dir);
}

std::string recognizeFile(const std::string& filename, std::string raw_dir)
{
    //定义了一个 smatch 类型的变量 m，用于保存正则表达式匹配结果。
    smatch m;
    //使用正则表达式来匹配文件名的后缀，判断文件类型是否为 doc、docx、pdf 或 xlsx。如果匹配成功，会进入相应的条件分支。
    if(regex_search(filename, m, regex(".(docx?)$")))
    {
        api_file_type = "02.docx";
    }
    else if(regex_search(filename, m, regex(".(pdf)$")))
    {
        api_file_type = "02.pdf";
    }
    else if(regex_search(filename, m, regex(".(xlsx)$")))
    {
        api_file_type = "02.xlsx";
    }
    else 
    {
        api_file_type= "02.jpg";
    }


    //打开指定路径的电子文件，以二进制只读模式打开，并将文件指针保存在 fp 变量中。
    FILE *fp = fopen(filename.c_str(), "rb");
    //定义了一个 vector 容器，用于存储读取的文件内容。
    vector<unsigned char> buf;
    if(fp)
    {
        //这几行代码用于读取文件内容。首先，通过 fseek 将文件指针定位到文件末尾，然后获取文件大小，并将容器 buf 调整为对应大小。
        //接着，将文件指针定位到文件开头，然后使用 fread 将文件内容读取到容器 buf 中。
        fseek(fp, 0, SEEK_END);
        buf.resize(ftell(fp));
        fseek(fp, 0, SEEK_SET);
        fread(buf.data(), buf.size(), 1, fp);
        fclose(fp);
    }
    //cout << "start" << endl;
    //调用另一个函数 recognizeBuf，用于对文件内容进行识别，并将识别结果保存在名为 ocrResult 的字符串变量中。
    //函数调用时，传递了读取的文件内容、文件大小以及识别结果保存的目录。
    std::string ocrResult = recognizeBuf(buf.data(), buf.size(), raw_dir);
    //cout << ocrResult << endl;
    return ocrResult;
}



void ReadFolder(const std::string folder, std::vector<std::string> &lines,

                std::vector<std::string> &file_name)
{
    if (folder == "")
    {
        std::cout << "folder name invalid, plz check." << std::endl;
        return;
    }

    DIR *pDir = opendir(folder.c_str());
    if (NULL == pDir)
    {
        std::cout << folder << " don't exist." << std::endl;
        return;
    }
    std::string prefix_name = folder;
    dirent *pDRent = NULL;
    while ((pDRent = readdir(pDir)) != NULL)
    {
        if (strlen(pDRent->d_name) > 3)
        {
            std::string postfix_name(pDRent->d_name);
            file_name.push_back(postfix_name);
            std::string full_path = prefix_name + "/" + postfix_name;
            lines.push_back(full_path);
        }
    }
    closedir(pDir);
}



bool newfolders(const std::string& foldername)
{
    if(!fs::exists(foldername))
        return fs::create_directories(foldername);
    return true;
}

//用于将结构化的JSON结果保存到指定文件中。它接受两个参数：filename表示要保存的文件名，strucured_result表示结构化的JSON格式字符串。
void persistenceJson(const std::string& filename, const std::string& strucured_result)
{
    std::ofstream os(filename, std::ofstream::out);
    os << strucured_result;
    os.close();
}




//用于对指定路径下的文件进行 OCR（光学字符识别）处理。
//接受两个参数：const std::string& scan_path 表示要扫描的路径，void *pContext 是一个无类型指针，可能是用于传递上下文信息的指针。
int test_engine(const std::string& scan_path, void *pContext)
{   //接下来的一系列代码用于构建一些目录路径，并通过调用 newfolders 函数创建这些目录。这些目录包括：
    //d_result_json: 用于存储识别结果的 JSON 文件。
    //d_result_txt: 用于存储识别结果的文本文件。
    //d_ocr: 用于存储 OCR 的原始输出。
    //d_dst_img: 用于存储目标图像。
    //d_meta: 用于存储 OCR 的元数据信息。
    std::string d_img = scan_path;
    std::string d_result_json = d_img +  "/result";
    std::string d_result_txt = d_img + "/result_txt";

    std::string d_ocr = d_img + "/raw_output";

    std::string d_dst_img = d_img + "/img_dir";

    std::string d_meta = d_img + "/meta";



    newfolders(d_result_json);

    newfolders(d_result_txt);

    newfolders(d_ocr);

    newfolders(d_dst_img);

    newfolders(d_meta);



    std::vector<std::string> file_path;

    std::vector<std::string> file_name;

    //这行代码调用 ReadFolder 函数，用于读取指定目录 d_img 下的所有文件的路径和文件名，并将其存储到前面定义的向量中。
    ReadFolder(d_img, file_path, file_name);



    // std::sort(file_path.begin(), file_path.end(), [](std::string& a, std::string& b){

    //       string str1 = a.substr(a.find_last_of('/')+1);

    //       string str2 = b.substr(b.find_last_of('/')+1);



    //       if(regex_search(str1, regex("^[0-9].*")) && regex_search(str2, regex("^[0-9].*")))

    //       {

    //           return std::stoi(str1) < std::stoi(str2);

    //       }

    //       return str1 < str2;

    // });




    //这是一个 for 循环，遍历之前获取到的文件路径 file_path 中的每个文件。
    for (auto& full_file_name : file_path)

    {
        //这行代码检查当前遍历到的路径是否是一个普通文件，如果不是，则跳过该路径，继续下一个。
        if (!fs::is_regular_file(full_file_name))

            continue;


    //根据全局变量 g_file_regex 的设置，判断当前文件路径是否匹配指定的文件名正则表达式。
    //如果 g_file_regex 不为空，并且当前文件路径不匹配正则表达式，则跳过该文件，继续下一个。
        if (g_file_regex.size() && !regex_search(full_file_name, regex(g_file_regex)))

        {

            continue;

        }


        // /这两行代码分别从完整的文件路径中提取文件名 filename 和去除扩展名后的纯文件名 pure_filename。
        string filename = fs::path(full_file_name).filename().generic_string();

        string pure_filename = fs::path(filename).replace_extension("").generic_string();


        //这行代码使用 OpenCV 的 imread 函数读取图像文件，将图像数据存储在 image 中。129 是 OpenCV 中的一个参数，用于指定图像的读取方式。
        cv::Mat image = cv::imread(full_file_name, 129);

        if (image.empty())

            continue;



        

        

        std::string fn_result_json = d_result_json + "/" + pure_filename + ".json";

        std::string fn_result_txt = d_result_txt + "/" + pure_filename + ".txt";

        std::string fn_dst_img = d_dst_img + "/" + filename;

        std::string fn_meta = d_meta + "/" + pure_filename + ".json";

        

        cout << "full_file_name:" << full_file_name << endl;


    //这行代码调用 recognizeFile 函数对当前文件进行 OCR 处理，OCR 的结果将保存在 ocr_result 字符串中。
        std::string ocr_result = recognizeFile(full_file_name, d_ocr);



        if (ocr_result == "-1")

        {

            std::cout << "ocr fail!" << std::endl;

            continue;

        }



        cout << fn_meta << endl;

        cout << fn_result_json << endl;

        cout << fn_result_txt << endl;

    }
    return 0;
}




int render(const std::string& filename, std::vector<cv::Mat>& images)
{
    if(std::regex_search(filename, std::regex("(pdf)$")))
    {
        //xwb::PDF2Img pdf2img;
        //pdf2img.convert(filename, images);
    }
    else if(std::regex_search(filename, std::regex("docx?$")))
    {
        //xwb::Word2Img word2img;
        //word2img.convert(filename, images);
    }
    else
    {
        cv::Mat image = cv::imread(filename, 1);
        images.push_back(image);
    }
    return 0;
}

int load_images(const std::string& folder, std::vector<cv::Mat>& images)
{
    int count = xwb::fs::countFiles(folder);

    for(int i = 0; i < count; i++)
    {
        std::string filename = folder + "/" + std::to_string(i) + ".jpg";

        if(xwb::fs::exist(filename))
        {
            cv::Mat image = cv::imread(filename, 1);
            images.push_back(image);
        }
    }

    return images.size();
}

std::string get_dir(const std::string& filename)
{
    std::experimental::filesystem::path p(filename);
    //这行代码使用 C++ 的 <filesystem> 头文件中的 path 类来处理文件路径。它将输入的文件路径 filename 构造成一个 path 对象 p。
    return p.parent_path().generic_string();
    //使用 path 对象的 parent_path() 方法，获取文件所在目录的路径
}

std::string renderOutputPath(const std::string& filename)
{
    std::string dir_name = get_dir(filename);
    //调用上面定义的 get_dir 函数，获取输入文件的所在目录路径，并将其保存在名为 dir_name 的字符串变量中。
    std::string md5_str = xwb::fs::fileMD5(filename);
    //调用了一个 xwb::fs 命名空间下的函数 fileMD5，用于计算指定文件的 MD5 值，并将结果保存在名为 md5_str 的字符串变量中。
    std::string output = dir_name + "/" + md5_str;

    std::cout << "filename: " << filename << endl;
    std::cout << "savepath: " << output << endl;

    return output;
}

int save_images(const std::string& dir_name, std::vector<cv::Mat>& images)
{
    newfolders(dir_name);
    for(int i = 0; i < images.size(); i++)
    {
        std::string filename = dir_name + "/" + std::to_string(i) + ".jpg";
        cv::imwrite(filename, images[i]);
    }
    return 0;
}


int render_or_load(const std::string& filename, std::vector<cv::Mat>& images)
{
    std::string save_path = renderOutputPath(filename);

    if(!xwb::fs::exist(save_path))
    {
        render(filename, images);
        save_images(save_path, images);
        images.clear();
    }
    
    load_images(save_path, images);
    return images.size();
}   







std::string recognize(cv::Mat& image)
{
    const std::string raw_dir = "/home/vincent/workspace/contract_compare/test-images/01/raw_output";
    newfolders(raw_dir);

    std::string ocr_result = recognizeMat(image, raw_dir);
    return ocr_result;
}

int batch_recognize(std::vector<cv::Mat>& images, std::vector<std::string>& ocr_results)
{
    for(auto& img : images)
    {
        std::string ocr_result = recognize(img);
        ocr_results.push_back(ocr_result);
    }
    return 0;
}


std::string concatOCRResult(const std::string& result1, const std::string& result2)
{
    nlohmann::json final_js = nlohmann::json::array();

    nlohmann::json doc1_js;
    doc1_js["doc_index"] = 1;
    doc1_js["pages"] = nlohmann::json::array();

    nlohmann::json page_js1 = nlohmann::json::parse(result1);
    doc1_js["pages"] = page_js1["result"]["pages"];


    nlohmann::json doc2_js;
    doc2_js["doc_index"] = 2;
    doc2_js["pages"] = nlohmann::json::array();

    nlohmann::json page_js2 = nlohmann::json::parse(result2);
    doc2_js["pages"] = page_js2["result"]["pages"];
    
    final_js.push_back(doc1_js);
    final_js.push_back(doc2_js);
    return final_js.dump();
}


std::string concatOCRResult(std::vector<std::string>& result1, std::vector<std::string>& result2)
{
    nlohmann::json final_js = nlohmann::json::array();

    nlohmann::json doc1_js;
    doc1_js["doc_index"] = 1;
    doc1_js["pages"] = nlohmann::json::array();

    for(int i = 0; i < result1.size(); i++)
    {
        nlohmann::json page_js = nlohmann::json::parse(result1[i]);
        page_js["page_index"] = i+1;
        page_js["status"] = 200;
        doc1_js["pages"].push_back(page_js);
    }

    nlohmann::json doc2_js;
    doc2_js["doc_index"] = 2;
    doc2_js["pages"] = nlohmann::json::array();

    for(int i = 0; i < result2.size(); i++)
    {
        nlohmann::json page_js = nlohmann::json::parse(result2[i]);
        page_js["page_index"] = i+1;
        page_js["status"] = 200;
        doc2_js["pages"].push_back(page_js);
    }

    final_js.push_back(doc1_js);
    final_js.push_back(doc2_js);
    return final_js.dump();
}

struct DifItem
{
    std::string text;
    std::string status;

    std::vector<int> polygon;
    std::vector<std::vector<int> > char_polygons;
    int page_index;
    int doc_index;

    DifItem()
        :page_index(0), doc_index(0){}
};

void parseDifItem(nlohmann::json& js, DifItem& item)
{
    item.text = js["text"].get<std::string>();
    item.status = js["status"].get<std::string>();
    item.page_index = js["page_index"].get<int>();
    item.doc_index = js["doc_index"].get<int>();
    item.polygon = js["polygon"].get<std::vector<int> >();
    item.char_polygons = js["char_polygons"].get<std::vector<std::vector<int> > >();
}

cv::Scalar statusColor(const std::string& status)
{
    if(status == "CHANGE")
    {
        return cv::Scalar(255); // 蓝色
    }
    else if(status == "INSERT")
    {
        return cv::Scalar(0, 255); // 绿色
    }
    else if(status == "DELETE") // 蓝色
    {
        return cv::Scalar(0, 0, 255); 
    }

    return cv::Scalar(128,128,128);
}

int visualResultDiff(const std::string& result, std::vector<cv::Mat> images1, std::vector<cv::Mat> images2)
{
    nlohmann::json js = nlohmann::json::parse(result);
    vector<float> rate1;
    vector<float> rate2;
    for(auto& image : images1)
    {
        float rate = 800.0 / image.cols;
        cv::resize(image, image, cv::Size(800, 800*image.rows/image.cols));
        rate1.push_back(rate);
    }

    for(auto& image : images2)
    {
        float rate = 800.0 / image.cols;
        cv::resize(image, image, cv::Size(800, 800*image.rows/image.cols));
        rate2.push_back(rate);
    }

    for(auto& item : js["result"]["detail"])
    {
        std::string status = item["status"].get<std::string>();
        std::string type = item["type"].get<std::string>();
        cv::Scalar color = statusColor(status);

        if(type == "text" || type == "stamp")
        {
            for(auto& c : item["diff"][0])
            {
                vector<int> polygon = c["polygon"].get<std::vector<int> >();
                int page_index = c["page_index"].get<int>();

                float rate = 1.0;
                if(rate1.size() > 0)
                    rate = rate1[page_index];
                if(images1.size() > 0)    
                    cv::rectangle(images1[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }
            for(auto& c : item["diff"][1])
            {
                vector<int> polygon = c["polygon"].get<std::vector<int> >();
                int page_index = c["page_index"].get<int>();
        
                float rate = 1.0;
                if(rate2.size() > 0)
                    rate = rate2[page_index];
                if(images2.size() > 0)
                    cv::rectangle(images2[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }
        }
        else if(type == "table" || type == "row" || type == "cell")
        {
            {
                vector<int> polygon = item["area"][0]["polygon"].get<std::vector<int> >();
                int page_index = item["area"][0]["page_index"].get<int>();
                float rate = 1.0;
                if(rate1.size() > 0)
                    rate = rate1[page_index];
                if(images1.size() > 0)
                    cv::rectangle(images1[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }

            {
                vector<int> polygon = item["area"][1]["polygon"].get<std::vector<int> >();
                int page_index = item["area"][1]["page_index"].get<int>();
                float rate = 1.0;
                if(rate1.size() > 0)
                    rate = rate1[page_index];
                if(images2.size() > 0)
                    cv::rectangle(images2[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }



            for(auto& d : item["diff"])
            {
                std::string status = d["status"].get<std::string>();
                cv::Scalar sub_color = statusColor(status);

                for(auto& c : d["diff"][0])
                {
                    vector<int> polygon = c["polygon"].get<std::vector<int> >();
                    int page_index = c["page_index"].get<int>();
                    float rate = 1.0;
                    if(rate1.size() > 0)
                        rate = rate1[page_index];
                    if(images1.size() > 0)
                        cv::rectangle(images1[page_index], 
                                    cv::Point(polygon[0] * rate, polygon[1] * rate),
                                    cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                    sub_color, 2);
                }
                for(auto& c : d["diff"][1])
                {
                    vector<int> polygon = c["polygon"].get<std::vector<int> >();
                    int page_index = c["page_index"].get<int>();
                    float rate = 1.0;
                    if(rate2.size() > 0)
                        rate = rate2[page_index];
                    if(images2.size() > 0)
                        cv::rectangle(images2[page_index], 
                                    cv::Point(polygon[0] * rate, polygon[1] * rate),
                                    cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                    sub_color, 2);
                }   
            }
        }
    }

    std::string save_path = g_root + "/visual_result/";
    newfolders(save_path);

    for(int i = 0; i < images1.size(); i++)
    {
        std::string filename = save_path + std::to_string(0) + "_" + std::to_string(i) + ".jpg";
        //cout << filename << endl;
        cv::imwrite(filename, images1[i]);
    }

    for(int i = 0; i < images2.size(); i++)
    {
        std::string filename = save_path + std::to_string(1) + "_" + std::to_string(i) + ".jpg";
        //cout << filename << endl;
        cv::imwrite(filename, images2[i]);
    }
    return 0;
}

int visualResult(const std::string& result, std::vector<cv::Mat> images1, std::vector<cv::Mat> images2)
{
    nlohmann::json js = nlohmann::json::parse(result);

    std::unordered_map<int, cv::Mat> m;


    for(auto& item : js["result"]["detail"])
    {
        DifItem item1;
        DifItem item2;

        parseDifItem(item[0], item1);
        parseDifItem(item[1], item2);

        if(item1.page_index == -1 || item2.page_index == -1)
        {
            item1.page_index = max(item1.page_index, item2.page_index);
            item2.page_index = max(item1.page_index, item2.page_index);
        }

        int key = (item1.page_index << 16) + item2.page_index;

        cv::Mat& img1 = images1[item1.page_index];
        cv::Mat& img2 = images2[item2.page_index];


        
        cv::Mat post_img1;
        cv::resize(img1, post_img1, cv::Size(800, 800*img1.rows/img1.cols));

        cv::Mat post_img2;
        cv::resize(img2, post_img2, cv::Size(800, 800*img2.rows/img2.cols));

        if(!m.count(key))
        {
            cv::Mat painter(max(post_img1.rows, post_img2.rows), post_img1.cols+post_img2.cols, CV_8UC3);
            post_img1.copyTo(painter(cv::Rect(0, 0, post_img1.cols, post_img1.rows)));
            post_img2.copyTo(painter(cv::Rect(post_img1.cols, 0, post_img2.cols, post_img2.rows)));
            m[key] = painter;
        }

        cv::Mat& painter = m[key];

        cv::Scalar color = statusColor(item1.status);

        //if(item1.text.size())
        for(auto& polygon : item1.char_polygons)
        {
            float rate = (float)post_img1.cols / img1.cols;
            rate *= 2;
            cv::rectangle(painter, 
                cv::Point(polygon[0] * rate, polygon[1] * rate),
                cv::Point((polygon[4]+2) * rate, polygon[5] * rate), 
                color, 2);
        }
        
        //if(item2.text.size())
        for(auto& polygon : item2.char_polygons)
        {
            float rate = (float)post_img2.cols / img2.cols;
            rate *= 2;
            cv::rectangle(painter, 
                cv::Point(post_img1.cols + polygon[0]*rate, polygon[1]*rate),
                cv::Point(post_img1.cols+ (polygon[4]+2)*rate, polygon[5]*rate), 
                color,2);
        }
    }

    std::string save_path = g_root + "/visual_result/";
    newfolders(save_path);

    for(auto it = m.begin(); it!= m.end(); it++)
    {
        int k = it->first;
        cv::Mat image = it->second;

        int i = k >>16;
        int j = k & 0xffff;

        std::string filename = save_path + std::to_string(i) + "_" + std::to_string(j) + ".jpg";
        cout << filename << endl;
        cv::imwrite(filename, image);
    }
    return 0;
}

//用于对单张图像进行视觉识别结果的可视化处理。它接受图像的索引字符串index_str、图像本身image以及对应的OCR（光学字符识别）结果字符串ocr_result作为输入参数。
int visual_recognize(const std::string& index_str, const cv::Mat& image, const std::string& ocr_result)
{
    cv::Mat painter = image.clone();

    nlohmann::json js = nlohmann::json::parse(ocr_result);

    std::map<std::string, cv::Scalar> area_colors = {
        {"paragraph", cv::Scalar(255)},
        {"list", cv::Scalar(0, 255)},
        {"edge", cv::Scalar(0, 0, 255)},
        {"table", cv::Scalar(255)},
        {"image", cv::Scalar(0, 0, 255)},
        {"stamp", cv::Scalar(0, 0, 255)},
        {"formula", cv::Scalar(0, 255, 0)},
        {"watermark", cv::Scalar(255)},
    };
//遍历JSON对象js中的每个区域（"areas"），并获取其中的位置信息（position）和区域类型（type）。
    for(auto& area_js : js["areas"])
    {
        vector<int> position = area_js["position"].get<std::vector<int> >();
        std::string type = area_js["type"].get<std::string>();
        for(int i = 0; i < 4; i++)
        {
            //对于每个区域，通过四个cv::line函数调用，绘制一个框，以标记该区域在图像中的位置。具体地，根据位置信息绘制四条边，形成一个矩形框。
            cv::line(painter, 
                cv::Point(position[(i*2)%8], position[((i*2+1)%8)]), 
                cv::Point(position[(i*2+2)%8], position[((i*2+3)%8)]),
                cv::Scalar(0, 0, 255), 2);
        }
        //如果区域类型是"table"，则在矩形框的左上角附近使用cv::putText函数绘制文字，将区域类型信息显示在图像中。
        if(type=="table")
        cv::putText(painter, type, cv::Point(position[4]+5, position[5]), cv::FONT_HERSHEY_SIMPLEX, 2.0, cv::Scalar(255));
    }

    for(auto& line_js : js["lines"])
    {
        vector<int> position = line_js["position"].get<std::vector<int> >();
        //std::string type = area_js["type"].get<std::string>();
        // for(int i = 0; i < 4; i++)
        // {
        //     cv::line(painter, 
        //         2*cv::Point(position[(i*2)%8], position[((i*2+1)%8)]), 
        //         2*cv::Point(position[(i*2+2)%8], position[((i*2+3)%8)]),
        //         area_colors["paragraph"], 2);
        // }

        //cv::putText(painter, type, cv::Point(position[4]+5, position[5]), cv::FONT_HERSHEY_SIMPLEX, 2.0, cv::Scalar(255));
    }

    //生成可视化结果后，将图像保存到指定位置，其中index_str用于生成文件名，保存在路径g_root + "/visual_ocr_result"下。
    std::string filename = g_root + "/visual_ocr_result";
    newfolders(filename);
    filename += "/" + index_str + ".jpg";

    cv::imwrite(filename, painter);
    return 0;
}

int batch_visual_recognize(const std::vector<cv::Mat>& images, std::vector<std::string>& ocr_results)
{
    static int index = 1;
    for(int i = 0; i < images.size(); i++)
    {
        std::string index_str = std::to_string(index) + "_" + std::to_string(i);
        visual_recognize(index_str, images[i], ocr_results[i]);
    }
    index++;
    return 0;
}

int batch_visual_recognize(const std::vector<cv::Mat>& images, std::string& ocr_result)
{
    nlohmann::json js = nlohmann::json::parse(ocr_result);

    static int index = 0;
    for(int i = 0; i < images.size(); i++)
    {
        std::string index_str = std::to_string(index) + "_" + std::to_string(i);
        std::string one_page_ocr_result = js["result"]["pages"][i].dump();
        //cout << one_page_ocr_result << endl;
        visual_recognize(index_str, images[i], one_page_ocr_result);
    }
    index++;
    return 0;
}


//这是一个函数定义，函数名为 debug_contract_compare，接受两个参数表示要比较的两个文件的路径。
int debug_contract_compare(const std::string& filename1, const std::string& filename2)
{
    //定义了两个空的 OpenCV 图像向量 images1 和 images2，用于存储后续处理的图像。
    std::vector<cv::Mat> images1;
    std::vector<cv::Mat> images2;
    //调用函数 render_or_load 对两个文件进行渲染或加载处理，将渲染后的图像存储在 images1 和 images2 中。
    render_or_load(filename1, images1);
    render_or_load(filename2, images2);

    //这两行代码定义了两个空的字符串向量 ocr_results1 和 ocr_results2，用于存储 OCR（光学字符识别）处理的结果。
    std::vector<std::string> ocr_results1;
    std::vector<std::string> ocr_results2;
    //调用函数 batch_recognize 对两个图像向量进行批量 OCR 处理，将处理的结果存储在 ocr_results1 和 ocr_results2 中。
    batch_recognize(images1, ocr_results1);
    batch_recognize(images2, ocr_results2);
    //调用函数 batch_visual_recognize 对两个图像向量和 OCR 处理结果进行批量可视化识别，但没有明确的返回值，可能是用于调试和观察处理结果。
    batch_visual_recognize(images1, ocr_results1);
    batch_visual_recognize(images2, ocr_results2);

    //调用函数 concatOCRResult 将两次 OCR 处理的结果拼接成一个字符串 allOcrResult。
    std::string allOcrResult = concatOCRResult(ocr_results1, ocr_results2);

    //创建了 xwb::RuntimeConfig 对象 config，用于配置合同比较运行时参数。
    xwb::RuntimeConfig config;
    std::string result;
    {
        //通过 TIME_COST_FUNCTION 宏记录合同比较的耗时
        //调用函数 xwb::contract_compare 对拼接后的 OCR 结果 allOcrResult 进行合同比较，将结果存储在 result 中。
        TIME_COST_FUNCTION;
        result  = xwb::contract_compare(allOcrResult, config);
    }
    
    //调用函数 persistenceJson 将输入 OCR 结果和合同比较结果分别存储到名为 "input.json" 和 "output.json" 的 JSON 文件中。
    persistenceJson("input.json", allOcrResult);
    persistenceJson("output.json", result);
    //调用函数 visualResult 对合同比较结果 result 和图像向量 images1、images2 进行可视化展示，可能是用于调试和观察处理结果。
    visualResult(result, images1, images2);

    return 0;
}


std::string recognize_document(const std::string& filename)
{
    std::string save_dir = g_root + "/raw_output/";
    newfolders(save_dir);
    //调用另一个函数 recognizeFile，用于对指定的电子文件 filename 进行识别操作。识别结果会保存在名为 ocr_result 的字符串变量中。
    std::string ocr_result = recognizeFile(filename, save_dir);

    return ocr_result;
}



//用于根据 OCR（光学字符识别）结果对图像进行缩放
//接受两个参数：std::string ocr_result 和 vector<cv::Mat>& images，前者表示 OCR 处理的结果字符串，后者是一个引用，用于存储处理后的图像。
int resize_image_base_ocr_result(std::string ocr_result, vector<cv::Mat>& images)
{
    // 这行代码将 OCR 处理结果字符串 ocr_result 解析为 JSON 对象 js，使用了 nlohmann::json 库进行解析。
    nlohmann::json js = nlohmann::json::parse(ocr_result);

    //遍历 OCR 处理结果中的每一页图像。
    for(int i = 0; i < js["result"]["pages"].size(); i++)
    {
        //这行代码获取当前页的 JSON 对象 js_page，包含了该页图像的相关信息，如宽度、高度和图像的 base64 编码字符串。
        nlohmann::json js_page = js["result"]["pages"][i];
        int dst_width = js_page["width"].get<int>();
        int dst_height = js_page["height"].get<int>();//从 js_page 中获取当前页图像的宽度和高度
        std::string img_base64_str = js_page["image_bytes"].get<std::string>();//获取当前页图像的 base64 编码字符串
        std::string img_binary_str = xwb::base64_decode(img_base64_str);//将 base64 编码的图像字符串解码为二进制字符串

        //这行代码将二进制字符串 img_binary_str 转换为 uint8_t 类型的向量 img_binary，用于存储图像的二进制数据。
        std::vector<uint8_t> img_binary((uint8_t*)img_binary_str.c_str(), (uint8_t*)img_binary_str.c_str() + img_binary_str.size());
        
        //调用 OpenCV 的 cv::imdecode 函数将图像的二进制数据解码为 OpenCV 的 cv::Mat 图像对象，并将其存储在 img_page 中。
        cv::Mat img_page = cv::imdecode(img_binary, 1);
        //这行代码将处理后的图像 img_page 添加到函数参数 images 所引用的图像向量中。
        images.push_back(img_page);
    }
    return 0;
}

int debug_contract_compare_electronic_file(const std::string& filename1, const std::string& filename2)
{
    //这是一个条件编译指令，如果宏 CV_DEBUG 已经定义，则下面的代码块会被包含在编译中，否则会被忽略。
    //这两行代码定义了两个 cv::Mat 类型的向量，用于存储电子文件的图像（用于调试用途）。
#ifdef CV_DEBUG
    std::vector<cv::Mat> images1;
    std::vector<cv::Mat> images2;

    // render_or_load(filename1, images1);
    // render_or_load(filename2, images2);
#endif
//分别对两个输入文件 filename1 和 filename2 进行 OCR（光学字符识别）操作，并将识别结果保存在名为 ocr_result1 和 ocr_result2 的字符串变量中。
    std::string ocr_result1 = recognize_document(filename1);
    std::string ocr_result2 = recognize_document(filename2);
//下面代码根据 OCR 结果调整图像大小，并将结果保存在 images1 和 images2 中（用于调试用途）。
#ifdef CV_DEBUG
    resize_image_base_ocr_result(ocr_result1, images1);
    resize_image_base_ocr_result(ocr_result2, images2);
    //调用 batch_visual_recognize 函数，对调整后的图像进行批量可视化识别
    batch_visual_recognize(images1, ocr_result1);
    batch_visual_recognize(images2, ocr_result2);
#endif
    //调用 concatOCRResult 函数，将两个 OCR 结果拼接成一个字符串，并将结果保存在名为 allOcrResult 的变量中。
    std::string allOcrResult = concatOCRResult(ocr_result1, ocr_result2);
    {
        //使用 JSON 库 nlohmann::json 解析两个 OCR 结果字符串，并将解析后的 JSON 对象保存在 js1 和 js2 变量中。
        nlohmann::json js1 = nlohmann::json::parse(ocr_result1);
        nlohmann::json js2 = nlohmann::json::parse(ocr_result2);
        //这行代码输出两个电子文件的页数信息，通过访问 JSON 对象的字段来获取页数信息。
        cout << "pages: " << js1["result"]["pages"].size() 
             << ", " << js2["result"]["pages"].size() << endl;
    }
    

    //调用 persistenceJson 函数，将拼接后的 OCR 结果保存到名为 input.json 的文件中。
    persistenceJson(g_root + "/input.json", allOcrResult);
    //创建了一个 xwb::RuntimeConfig 对象 config，并设置了其中的几个参数。
    xwb::RuntimeConfig config;
    config.block_compare = false;
    config.ignore_punctuation = true;
    config.merge_diff = false;
    std::string result;
    {
        TIME_COST_FUNCTION;
        //调用 xwb::contract_compare 函数，比较两个电子文件的内容差异，并将结果保存在名为 result 的字符串变量中。TIME_COST_FUNCTION 是一个用于记录函数执行时间的宏。
        result  = xwb::contract_compare(allOcrResult, config);
    }
    

    //这行代码调用 persistenceJson 函数，将比较结果保存到名为 output.json 的文件中。
    persistenceJson(g_root + "/output.json", result);
    //这是条件编译指令的结束标记，用于结束条件编译块。
#ifdef CV_DEBUG
    //visualResult(result, images1, images2);
    visualResultDiff(result, images1, images2);
#endif
    return 0;
}

void testInputFile()
{
    std::string filename = "/home/vincent/workspace/contract_compare_ted/test-images/43/xls.json";
    ifstream ifs(filename);
    nlohmann::json js;
    ifs >> js;
    xwb::RuntimeConfig config;
    config.ignore_punctuation = true;
    config.punctuation = "^.,;:。?!､'“”‘’()[]{}《》〈〉—_*×□/~→+-><=•、`|~~¢£¤¥§©«®°±»àáèéìíòó÷ùúāēěīōūǎǐǒǔǘǚǜΔΣΦΩ฿‰₣₤₩₫€₰₱₳₴℃℉≈≠≤≥①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳■▶★☆☑☒✓々「」『』【】〒〖〗〔〕㎡㎥・･…．";
    
    //这行代码调用合同比对的函数 xwb::contract_compare，传递了 JSON 对象 js 的字符串表示以及配置对象 config 作为参数，并将比对结果存储在 result 变量中。
    std::string result  = xwb::contract_compare(js.dump(), config);
    cout << result << endl;
    return;
}

void createDebugFolders()
{
    std::string dir_name = g_root + "/paragraph_segment/";
    newfolders(dir_name);

    dir_name = g_root + "/semantic_seg/";
    newfolders(dir_name);
}

void test_diff()
{
    // using namespace diff;
    std::wstring a = L"金额：10.0"
"一、合同标的"
"货物名称、具体规格型号、数量,见.附表。....."
"kaishi货物dd名称、具体规格型号、数量,见.yyy附表。.....";
        std::wstring b = L"金额：1.0"
"一、合同标的"
"货物名称、具体规格型号、数量,见.附表。....."
"kaishi货物xx名称、具体规格型号、数量,见.xxx附表。.....";
    using namespace xwb;
    std::vector<Diff> diffs = diff_main(a, b);
    // vector<Diff> diffs = diff_main(a, b);

    for(auto& d : diffs)
    {
        cout << d.operation << ", " << to_utf8(d.text) << endl;
    }
}


int main(int argc, char **argv)
{
    cout << xwb::get_version() << endl;
    // testInputFile();
    //  return 0;

    loadConfig("/home/luyjunbo-10gbra/桌面/ca/contract_compare/demo/runtime_config.yaml");

    std::string fn_doc1 = "/home/luyjunbo-10gbra/桌面/ca/a1.png";
    std::string fn_doc2 = "/home/luyjunbo-10gbra/桌面/ca/a2.png";

    // fn_doc1 = argv[1];
    // fn_doc2 = argv[2];

    g_root = get_dir(fn_doc1);
    createDebugFolders();
    
    debug_contract_compare_electronic_file(fn_doc1, fn_doc2);
    cout << "Finished!" << endl;
    return 0;
}