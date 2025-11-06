#!/bin/bash
# 完整修复Spine 4.0从cocos2d到axmol的API差异(基于官方提交历史)

SPINE_DIR="./spine/src/spine"

echo "========================================="
echo "开始修复Spine 4.0 API (基于官方提交历史)"
echo "========================================="
echo ""

# 1. 替换 cocos2d.h → axmol.h
echo "[1/16] 替换 #include cocos2d.h → axmol.h"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/#include "cocos2d\.h"/#include "axmol.h"/g' {} \;

# 2. 替换 USING_NS_CC → using namespace ax (注意:不是USING_NS_AX)
echo "[2/16] 替换 USING_NS_CC → using namespace ax"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/USING_NS_CC/using namespace ax/g' {} \;
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/USING_NS_AX/using namespace ax/g' {} \;

# 3. 替换 AX_DEPRECATED_ATTRIBUTE → AX_DEPRECATED(2.1)
echo "[3/16] 替换 AX_DEPRECATED_ATTRIBUTE → AX_DEPRECATED(2.1)"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/AX_DEPRECATED_ATTRIBUTE/AX_DEPRECATED(2.1)/g' {} \;
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/CC_DEPRECATED_ATTRIBUTE/AX_DEPRECATED(2.1)/g' {} \;

# 4. 删除 AX_CONSTRUCTOR_ACCESS : (包括冒号)
echo "[4/16] 删除 AX_CONSTRUCTOR_ACCESS :"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/AX_CONSTRUCTOR_ACCESS ://g' {} \;
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/CC_CONSTRUCTOR_ACCESS ://g' {} \;

# 5. 替换 cocos2d:: → ax::
echo "[5/16] 替换 cocos2d:: → ax::"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/cocos2d::/ax::/g' {} \;

# 6. 替换 axmol:: → ax:: (如果有的话)
echo "[6/16] 统一 axmol:: → ax::"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/axmol::/ax::/g' {} \;

# 7. 替换 spine-cocos2dx.h → spine-axmol.h
echo "[7/16] 替换 #include spine-cocos2dx.h → spine-axmol.h"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/#include <spine\/spine-cocos2dx\.h>/#include <spine\/spine-axmol.h>/g' {} \;

# 8. 替换类名前缀 Cocos2d → Axmol
echo "[8/16] 替换类名 Cocos2d前缀 → Axmol"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/Cocos2dAtlasAttachmentLoader/AxmolAtlasAttachmentLoader/g' {} \;
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/Cocos2dTextureLoader/AxmolTextureLoader/g' {} \;
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/Cocos2dExtension/AxmolExtension/g' {} \;

# 9. 替换宏 CCASSERT → AXASSERT
echo "[9/16] 替换 CCASSERT → AXASSERT"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/CCASSERT/AXASSERT/g' {} \;

# 10. 替换 Color4F → Color4B (DrawNode API变化)
echo "[10/16] 替换 Color4F → Color4B"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/Color4F::/Color4B::/g' {} \;
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/\bColor4F\b/Color4B/g' {} \;

# 11. 删除 setLineWidth 调用
echo "[11/16] 删除 drawNode->setLineWidth() 调用"
find "$SPINE_DIR" -type f -name "*.cpp" -exec sed -i '/drawNode->setLineWidth/d' {} \;

# 12. 修复 drawPoly 调用 (添加lineWidth参数)
echo "[12/16] 修复 drawPoly 调用添加 lineWidth 参数"
find "$SPINE_DIR" -type f -name "*.cpp" -exec sed -i 's/drawPoly(\([^)]*\), true, \(Color4B::[A-Z]*\))/drawPoly(\1, true, \2, 2.0f)/g' {} \;

# 13. 修复 drawLine 调用 (添加lineWidth参数)
echo "[13/16] 修复 drawLine 调用添加 lineWidth 参数"
find "$SPINE_DIR" -type f -name "*.cpp" -exec sed -i 's/drawLine(\([^)]*\), Color4B::\([A-Z]*\))/drawLine(\1, Color4B::\2, 2.0f)/g' {} \;

# 14. 确保单独的类型有ax::前缀
echo "[14/16] 添加 ax:: 前缀到常用类型"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/\bTexture2D\*/ax::Texture2D*/g' {} \;
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/\bRenderer\*/ax::Renderer*/g' {} \;

# 15. 修复可能的双重ax::前缀
echo "[15/16] 清理双重 ax:: 前缀"
find "$SPINE_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i 's/ax::ax::/ax::/g' {} \;

# 16. 修复 AX_SPINE_VERSION (如果存在)
echo "[16/16] 检查 AX_SPINE_VERSION"
# spine 4.0应该是 0x040000
find "$SPINE_DIR" -type f -name "*.h" -exec sed -i 's/#define AX_SPINE_VERSION 0x.*/#define AX_SPINE_VERSION 0x040000/g' {} \;

echo ""
echo "========================================="
echo "所有API修复完成!"
echo "========================================="
echo ""
echo "修改摘要:"
echo "- 头文件: cocos2d.h → axmol.h"
echo "- 命名空间: USING_NS_CC → using namespace ax"
echo "- 宏: AX_DEPRECATED_ATTRIBUTE → AX_DEPRECATED(2.1)"
echo "- 类名: Cocos2d → Axmol"
echo "- DrawNode: Color4F → Color4B, 添加 lineWidth 参数"
echo "- 删除: AX_CONSTRUCTOR_ACCESS, setLineWidth()"
echo ""
echo "下一步: 重新配置CMake并编译"
echo "  cd ../../projects/SlotGame"
echo "  cmake -B build -G \"Visual Studio 17 2022\" -A x64"
echo "  cmake --build build --config Debug --target spine"
echo ""
