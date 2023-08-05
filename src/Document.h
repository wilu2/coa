#pragma once

#include "Page.h"
#include "nlohmann/my_json.hpp"
#include "contract_compare.h"


namespace xwb
{
    
class Document
{
public:
    Document()
    {
        total_page_number = -1;
        char_num = 0;
        doc_index = 0;
    }
    //加载文档的所有页，并解析文档的信息。传入的 js 参数是一个 JSON 对象，包含文档的相关信息。
    int load_pages(my_json& js, int& origin_char_index, Context& context);
    //加载文档的所有页，并解析文档的信息，同时获取文档的原始文本内容。
    int load_pages_with_layout(my_json& js, std::wstring& origin_text, int& origin_char_index, Context& context);
    //void layout_analyze()：分析文档的布局，即划分文档中的内容块，如页眉、页脚、正文等。
    void layout_analyze();
    //void mark_header()：标记文档的页眉。
    void mark_header();
    //void mark_footer()：标记文档的页脚。
    void mark_footer();
    //生成文档的段落。传入的 page 参数是一个 Page 类的对象，表示文档中的一页。函数返回一个二维向量，每个元素都是一个 TextLine 类的对象，表示文档的段落信息。
    std::vector<std::vector<TextLine> > generate_paragraphs(Page& page);
    //void paragraph_segment()：分段处理文档。
    void paragraph_segment();
    //void semantic_seg()：对文档进行语义分析。
    void semantic_seg();
    //void semantic_seg()：对文档进行语义分析。
    void get_orgin_text(std::wstring& origin_text);


public:
    std::vector<Page> pages;//表示文档的页列表，每个页都是 Page 类的对象，用于存储页的信息。
    int total_page_number;//表示文档的总页数。
    std::vector<Page> paragraphs;//表示文档的段落列表，每个段落都是由一组 TextLine 类的对象组成，用于存储段落的信息。
    int char_num;//表示文档中的字符数量。
    int doc_index;//表示文档的索引。
};



} // namespace xwb
