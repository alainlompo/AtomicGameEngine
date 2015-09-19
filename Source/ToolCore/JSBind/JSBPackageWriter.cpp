//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#include <Atomic/IO/FileSystem.h>

#include "JSBind.h"
#include "JSBModule.h"
#include "JSBPackage.h"
#include "JSBEnum.h"
#include "JSBClass.h"

#include "JSBPackageWriter.h"

namespace ToolCore
{

JSBPackageWriter::JSBPackageWriter(JSBPackage *package) : package_(package)
{

}

void JSBPackageWriter::WriteProtoTypeRecursive(String &source, JSBClass* klass,  Vector<JSBClass*>& written)
{
    if (written.Contains(klass))
        return;

    PODVector<JSBClass*>& baseClasses = klass->GetBaseClasses();

    Vector<JSBClass*>::Iterator itr = baseClasses.End() - 1 ;

    while (itr != baseClasses.Begin() - 1)
    {
        WriteProtoTypeRecursive(source, (*itr), written);
        itr--;
    }

    JSBClass* base = baseClasses.Size() ? baseClasses[0] : NULL;

    if (!klass->IsNumberArray() && klass->GetPackage() == package_)
    {
        JSBModule* module = klass->GetModule();

        if (module->Requires("3D"))
            source += "\n#ifdef ATOMIC_3D\n";

        String packageName =  klass->GetModule()->GetPackage()->GetName();
        String basePackage =  base ? base->GetModule()->GetPackage()->GetName() : "";

        source.AppendWithFormat("   js_setup_prototype(vm, \"%s\", \"%s\", \"%s\", \"%s\", %s);\n",
                                packageName.CString(), klass->GetName().CString(),
                                base ? basePackage.CString() : "", base ? base->GetName().CString() : "",
                                klass->HasProperties() ? "true" : "false");

        if (module->Requires("3D"))
            source += "#endif\n\n";
    }

    written.Push(klass);

}

void JSBPackageWriter::WriteProtoTypeSetup(String& source)
{
    Vector<JSBClass*> written;

    PODVector<JSBClass*>& allClasses = package_->GetAllClasses();

    for (unsigned i = 0; i < allClasses.Size(); i++)
    {
        WriteProtoTypeRecursive(source, allClasses[i], written);
    }
}

void JSBPackageWriter::GenerateSource(String& sourceOut)
{
    String source = "// This file was autogenerated by JSBind, changes will be lost\n\n";
    source += "#include <Duktape/duktape.h>\n";
    source += "#include <AtomicJS/Javascript/JSVM.h>\n";
    source += "#include <AtomicJS/Javascript/JSAPI.h>\n";

    source += "\n\nnamespace Atomic\n{\n";

    String packageLower = package_->GetName().ToLower();

    for (unsigned i = 0; i < package_->modules_.Size(); i++)
    {
        JSBModule* module = package_->modules_.At(i);

        String moduleLower = module->GetName().ToLower();

        source.AppendWithFormat("\nextern void jsb_package_%s_preinit_%s (JSVM* vm);", packageLower.CString(), moduleLower.CString());
        source.AppendWithFormat("\nextern void jsb_package_%s_init_%s (JSVM* vm);", packageLower.CString(), moduleLower.CString());
    }

    source += "\n\nstatic void jsb_modules_setup_prototypes(JSVM* vm)\n{\n";

    source += "   // It is important that these are in order so the prototypes are created properly\n";
    source += "   // This isn't trivial as modules can have dependencies, so do it here\n\n";

    WriteProtoTypeSetup(source);

    source += "\n}\n";

    source.AppendWithFormat("\n\nstatic void jsb_package_%s_preinit(JSVM* vm)\n{", packageLower.CString());


    source.Append("\n    // Create the global package object\n");
    source.Append("    duk_context* ctx = vm->GetJSContext();\n");
    source.Append("    duk_push_object(ctx);\n");
    source.AppendWithFormat("    duk_put_global_string(ctx, \"%s\");\n", package_->GetName().CString());

    for (unsigned i = 0; i < package_->modules_.Size(); i++)
    {
        JSBModule* module = package_->modules_.At(i);

        if (module->Requires("3D"))
            source += "\n#ifdef ATOMIC_3D";

        String moduleLower = module->GetName().ToLower();

        source.AppendWithFormat("\n   jsb_package_%s_preinit_%s(vm);", packageLower.CString(), moduleLower.CString());

        if (module->Requires("3D"))
            source += "\n#endif //ATOMIC_3D\n";
    }

    source += "\n}\n\n";

    source.AppendWithFormat("\n\nvoid jsb_package_%s_init(JSVM* vm)\n{", packageLower.CString());

    source.AppendWithFormat("\n\n   jsb_package_%s_preinit(vm);\n", packageLower.CString());

    source += "\n\n   jsb_modules_setup_prototypes(vm);\n";

    for (unsigned i = 0; i < package_->modules_.Size(); i++)
    {
        JSBModule* module = package_->modules_.At(i);

        String moduleLower = module->GetName().ToLower();

        if (module->Requires("3D"))
            source += "\n#ifdef ATOMIC_3D";
        source.AppendWithFormat("\n   jsb_package_%s_init_%s(vm);", packageLower.CString(), moduleLower.CString());
        if (module->Requires("3D"))
            source += "\n#endif //ATOMIC_3D\n";
    }

    source += "\n}\n\n";

    // end Atomic namespace
    source += "\n}\n";

    sourceOut = source;

}

}
