import os
import xml.etree.ElementTree as ET

vcxproj_path = os.path.join(os.path.dirname(__file__), 'vs', 'nginx.vcxproj')
tree = ET.parse(vcxproj_path)
root = tree.getroot()
namespaces = {'ns': 'http://schemas.microsoft.com/developer/msbuild/2003'}
ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')

for idg in root.findall('ns:ItemDefinitionGroup', namespaces):
    if 'Condition' in idg.attrib and 'Win32' in idg.attrib['Condition']:
        for link in idg.findall('ns:Link', namespaces):
            deps = link.find('ns:AdditionalDependencies', namespaces)
            if deps is not None:
                deps.text = '$(DepsDir)\\pcre2_x86\\lib\\pcre2-8-static.lib;$(DepsDir)\\zlib_x86\\lib\\zlibstatic.lib;$(DepsDir)\\openssl_x86\\lib\\VC\\x86\\MT\\libssl.lib;$(DepsDir)\\openssl_x86\\lib\\VC\\x86\\MT\\libcrypto.lib;kernel32.lib;user32.lib;ws2_32.lib;mswsock.lib;advapi32.lib;crypt32.lib;%(AdditionalDependencies)'

tree.write(vcxproj_path, xml_declaration=True, encoding='utf-8')
