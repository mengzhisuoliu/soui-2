# ChangeLog file for SOUI
## 5.0 ����swinx�⣬ʵ�ֶԿ�ƽ̨��֧�֣�ͬʱ�����˲��ֿؼ���

## 4.0 ʹ����COM��������SOUI���Ķ��󣬷���C���Ե��á�
### 3.0����4.0ע������
1.  ԭSOUI_CLASS_NAME�滻ΪDEF_SOBJECT, �ҵ�һ�����������Ļ���
2.  �¼�����ʹ��DEF_EVENT/DEF_EVENT_EXT�������������壬ʹ��ʹ�÷����μ�ϵͳ�¼�
3.  ���õ�ResProviderʹ��SouiFactory�������������ṩȫ�ֺ���������
4.  resbuilder��д����R,UIRES�ķ����е�仯���ο�demo
5.  SEventArgs�滻ΪIEvtArgs
6. ԭ������ֱ��ʹ��pugi_xml��4.0��ͳһʹ��SOUI��װ���SXmlDoc, SXmlNode, SXmlAttribute����Ӧ�Ľӿ�ΪIXmlDoc, IXmlNode, IXmlAttribute��
7. ԭ������ʹ�õ�IPen, IBrush, IBitmap, IRegion, IPath��IRenderObj����ͳһ����S��׺����ΪIPenS, IBrushS, IBitmapS, IRegionS, IPathS�ȣ���Ҫ��C�ӿ��в�֧�������ռ䣬���¿��ܺ�ϵͳ�ӿ��������������޸���C++�汾�������˼��ݶ��壩
8. Ϊ�˼���C�ӿڣ����нӿڵķ�����û����Ĭ�ϲ���������IRenderTarget.SelectObject��3.0�ڶ�������Ĭ��ΪNULL, �°汾û��Ĭ�ϲ���������дȫ������(�����޸��Ѿ����Լ���Ĭ�ϲ�����
9. XxxView���������ӿ��е��������з�����ͳһʹ��WINAPI���ã�ԭSWindow*�Ĳ����滻ΪSItemPanel*, ԭpugi::xml_node�滻ΪSXmlNode��
10. SHostWnd��ֱ�Ӽ̳���SWindow������ֻ�̳�SNativeWnd, ��Ҫʹ��GetRoot()����ȡ����SWindow*
11. SXmlDoc�����pugi::xml_documentһ����������SXmlDoc���̳�SXmlNode, Ҫ��ȡroot�ڵ㣬����ʹ��SXmlDoc.root()������
12. SStringX.Format��ʹ��%s����һ��SStringX��������ʽ��ʱ��������3.0һ��ֱ��ʹ��SStringX���ڲ���������Ǳ���Ҫʹ��SStringX.c_str()

## 3.0 ���Ӿ���任����Android����
## 2.0 ����SOUIģ��ṹ���