using System.Globalization;
using System.IO.Compression;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.RegularExpressions;
using ClangSharp.Interop;
using static ClangSharp.Interop.CXCursorKind;
using static ClangSharp.Interop.CXTypeKind;
using static ClangSharp.Interop.CXChildVisitResult;

internal sealed class Job
{
    public string Source { get; set; } = "";
    public string Output { get; set; } = "";
    public string NativeLibraries { get; set; } = "";
    public string[] Arguments { get; set; } = [];
    public string[] Roots { get; set; } = [];
    public string MacroNames { get; set; } = "";
    public string[] ProbeNames { get; set; } = [];
    public string Mode { get; set; } = "extract";
}

internal sealed class Snapshot
{
    public string Source { get; set; } = "";
    public string[] Arguments { get; set; } = [];
    public string Version { get; set; } = "";
    public int ParseCode { get; set; }
    public List<Diagnostic> Diagnostics { get; set; } = [];
    public List<Symbol> Symbols { get; set; } = [];
    public List<TypeInfo> Types { get; set; } = [];
    public List<string> Files { get; set; } = [];
    public double Seconds { get; set; }
    public string? Failure { get; set; }
}

internal sealed record Site(string File, uint Line, uint Column);
internal sealed record Diagnostic(int Severity, string Message, Site Site);
internal sealed record Parameter(string Name, int Type, Site Site, string? Raw);
internal sealed class Symbol
{
    public string Kind { get; set; } = "";
    public string Name { get; set; } = "";
    public string Qualified { get; set; } = "";
    public string Usr { get; set; } = "";
    public string Owner { get; set; } = "";
    public string OwnerUsr { get; set; } = "";
    public Site Site { get; set; } = new("", 0, 0);
    public int Type { get; set; } = -1;
    public int Result { get; set; } = -1;
    public List<Parameter> Parameters { get; set; } = [];
    public List<int> Bases { get; set; } = [];
    public long OffsetBits { get; set; } = -1;
    public int BitWidth { get; set; } = -1;
    public int Order { get; set; } = -1;
    public bool Definition { get; set; }
    public bool Anonymous { get; set; }
    public bool Virtual { get; set; }
    public bool Pure { get; set; }
    public bool Const { get; set; }
    public bool FunctionLike { get; set; }
    public string? Uuid { get; set; }
    public object? Value { get; set; }
    public string[]? Tokens { get; set; }
    public string? Raw { get; set; }
}

internal sealed class TypeInfo
{
    public string Written { get; set; } = "";
    public string Canonical { get; set; } = "";
    public string Kind { get; set; } = "";
    public string Declaration { get; set; } = "";
    public string DeclarationUsr { get; set; } = "";
    public long Size { get; set; } = -1;
    public long Alignment { get; set; } = -1;
    public bool Const { get; set; }
    public bool Volatile { get; set; }
    public bool Restrict { get; set; }
    public int Element { get; set; } = -1;
    public long Count { get; set; } = -1;
    public int Result { get; set; } = -1;
    public List<int> Arguments { get; set; } = [];
    public string CallingConvention { get; set; } = "";
    public string ExceptionSpecification { get; set; } = "";
    public bool Variadic { get; set; }
}

internal static class Program
{
    internal static readonly JsonSerializerOptions Json = new() { PropertyNamingPolicy = JsonNamingPolicy.CamelCase, PropertyNameCaseInsensitive = true };
    internal static string Text(CXString text) { using (text) return text.ToString(); }

    public static int Main(string[] args)
    {
        if (args.Length != 1) { Console.Error.WriteLine("用法：Extractor.exe <任务.json>"); return 1; }
        var job = JsonSerializer.Deserialize<Job>(File.ReadAllText(args[0]), Json)!;
        clang.ResolveLibrary += (name, assembly, path) => NativeLibrary.Load(Path.Combine(job.NativeLibraries, name + ".dll"));
        var watch = System.Diagnostics.Stopwatch.StartNew();
        var snapshot = new Snapshot { Source = job.Source, Arguments = job.Arguments };
        try
        {
            snapshot.Version = Text(clang.getClangVersion());
            using var index = CXIndex.Create(false, false);
            var flags = CXTranslationUnit_Flags.CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_Flags.CXTranslationUnit_IncludeAttributedTypes |
                CXTranslationUnit_Flags.CXTranslationUnit_VisitImplicitAttributes | CXTranslationUnit_Flags.CXTranslationUnit_SkipFunctionBodies;
            var code = CXTranslationUnit.TryParse(index, job.Source, job.Arguments, ReadOnlySpan<CXUnsavedFile>.Empty, flags, out var unit);
            snapshot.ParseCode = (int)code;
            if (code == CXErrorCode.CXError_Success)
            {
                using (unit)
                {
                    var extractor = new Extractor(job, unit, snapshot);
                    for (uint i = 0; i < unit.NumDiagnostics; i++)
                    {
                        using var diagnostic = unit.GetDiagnostic(i);
                        snapshot.Diagnostics.Add(new((int)diagnostic.Severity, Text(diagnostic.Format(CXDiagnostic.DefaultDisplayOptions)), extractor.Location(diagnostic.Location)));
                    }
                    extractor.Walk(unit.Cursor);
                }
            }
        }
        catch (Exception exception) { snapshot.Failure = exception.ToString(); }
        snapshot.Seconds = watch.Elapsed.TotalSeconds;
        Directory.CreateDirectory(Path.GetDirectoryName(job.Output)!);
        using (var output = new GZipStream(File.Create(job.Output + ".tmp"), CompressionLevel.Fastest)) JsonSerializer.Serialize(output, snapshot, Json);
        File.Move(job.Output + ".tmp", job.Output, true);
        Console.WriteLine(JsonSerializer.Serialize(new { source = job.Source, symbols = snapshot.Symbols.Count, types = snapshot.Types.Count, errors = snapshot.Diagnostics.Count(d => d.Severity >= 3), snapshot.Seconds, snapshot.Failure }, Json));
        return snapshot.Failure != null || snapshot.ParseCode != 0 ? 1 : 0;
    }
}

internal sealed unsafe class Extractor(Job job, CXTranslationUnit unit, Snapshot snapshot)
{
    readonly Dictionary<CXFile, string> files = [];
    readonly Dictionary<CXType, int> types = [];
    readonly Dictionary<string, int> orders = [];
    readonly Dictionary<CXCursor, string> names = [];
    readonly Dictionary<CXFile, byte[]> sourceFiles = [];
    readonly HashSet<string> macroNames = File.Exists(job.MacroNames) ? File.ReadAllLines(job.MacroNames).ToHashSet(StringComparer.Ordinal) : [];
    readonly HashSet<string> visited = [];
    Exception? failure;

    public Site Location(CXSourceLocation location)
    {
        location.GetExpansionLocation(out var file, out var line, out var column, out _);
        if (!files.TryGetValue(file, out var path))
        {
            path = Program.Text(file.Name).Replace('\\', '/');
            files[file] = path;
            if (!string.IsNullOrEmpty(path)) snapshot.Files.Add(path);
        }
        return new(path, line, column);
    }

    bool Included(string path) => job.Roots.Any(root => path.StartsWith(root.Replace('\\', '/').TrimEnd('/') + "/", StringComparison.OrdinalIgnoreCase));

    string Name(CXCursor cursor)
    {
        if (cursor.IsNull || cursor.kind == CXCursor_TranslationUnit) return "";
        if (names.TryGetValue(cursor, out var found)) return found;
        var leaf = Program.Text(cursor.Spelling);
        if (leaf.Length == 0) { var site = Location(cursor.Location); leaf = $"(anonymous@{site.Line}:{site.Column})"; }
        var parent = cursor.SemanticParent;
        var prefix = parent.IsNull || parent.kind == CXCursor_TranslationUnit || parent == cursor ? "" : Name(parent);
        return names[cursor] = prefix.Length == 0 ? leaf : prefix + "::" + leaf;
    }

    int Type(CXType source)
    {
        if (source.kind == CXType_Invalid) return -1;
        if (types.TryGetValue(source, out var existing)) return existing;
        var canonical = source.CanonicalType;
        var value = new TypeInfo { Written = Program.Text(source.Spelling), Canonical = Program.Text(canonical.Spelling), Kind = canonical.kind.ToString().Replace("CXType_", ""),
            Size = canonical.SizeOf, Alignment = canonical.AlignOf, Const = canonical.IsConstQualified, Volatile = canonical.IsVolatileQualified, Restrict = canonical.IsRestrictQualified };
        var id = snapshot.Types.Count;
        types.Add(source, id);
        snapshot.Types.Add(value);
        switch (canonical.kind)
        {
            case CXType_Pointer: case CXType_LValueReference: case CXType_RValueReference: case CXType_BlockPointer:
                value.Element = Type(clang.getPointeeType(canonical)); break;
            case CXType_ConstantArray: case CXType_IncompleteArray: case CXType_VariableArray: case CXType_DependentSizedArray:
                value.Element = Type(canonical.ArrayElementType); value.Count = canonical.ArraySize; break;
            case CXType_Record: case CXType_Enum:
                var declaration = clang.getTypeDeclaration(canonical);
                value.Declaration = Name(declaration); value.DeclarationUsr = Program.Text(declaration.Usr);
                if (canonical.kind == CXType_Enum) value.Element = Type(declaration.EnumDecl_IntegerType);
                break;
            case CXType_FunctionProto: case CXType_FunctionNoProto:
                value.Result = Type(canonical.ResultType);
                for (uint i = 0; i < canonical.NumArgTypes; i++) value.Arguments.Add(Type(canonical.GetArgType(i)));
                value.CallingConvention = canonical.FunctionTypeCallingConv.ToString();
                value.ExceptionSpecification = canonical.ExceptionSpecificationType.ToString();
                value.Variadic = canonical.IsFunctionTypeVariadic;
                break;
        }
        return id;
    }

    List<CXCursor> Children(CXCursor cursor)
    {
        var children = new List<CXCursor>();
        cursor.VisitChildren((child, parent, data) => { children.Add(child); return CXChildVisit_Continue; }, default);
        return children;
    }

    object? StringValue(CXCursor cursor, int depth = 0)
    {
        if (depth > 16 || cursor.IsNull) return null;
        if (cursor.kind == CXCursor_VarDecl) return StringValue(cursor.InitExpr, depth + 1);
        if (cursor.kind == CXCursor_StringLiteral)
        {
            var width = cursor.Type.ArrayElementType.SizeOf;
            var length = cursor.Type.ArraySize - 1;
            if (width <= 0 || length < 0 || width * length > 16 * 1024 * 1024) return null;
            using var literal = cursor.StringLiteralValue;
            var pointer = clang.getCString(literal);
            if (pointer == null) return null;
            return new { kind = "string_bytes", elementWidth = width, length, bytes = Convert.ToHexString(new ReadOnlySpan<byte>(pointer, checked((int)(width * length)))) };
        }
        if (cursor.kind is CXCursor_UnexposedExpr or CXCursor_ParenExpr or CXCursor_CStyleCastExpr or CXCursor_CXXStaticCastExpr or CXCursor_CXXConstCastExpr)
        {
            var children = Children(cursor).Where(c => clang.isExpression(c.kind) != 0).ToArray();
            if (children.Length == 1) return StringValue(children[0], depth + 1);
        }
        if (cursor.kind == CXCursor_DeclRefExpr && !cursor.Referenced.IsNull) return StringValue(cursor.Referenced, depth + 1);
        return null;
    }

    object? Value(CXCursor cursor, int depth = 0)
    {
        if (depth > 16 || cursor.IsNull) return null;
        var stringValue = StringValue(cursor);
        if (stringValue != null) return stringValue;
        using var evaluation = cursor.Evaluate;
        if (evaluation.Handle != IntPtr.Zero)
        {
            switch (evaluation.Kind)
            {
                case CXEvalResultKind.CXEval_Int: return new { kind = "integer", text = evaluation.IsUnsignedInt ? evaluation.AsUnsigned.ToString(CultureInfo.InvariantCulture) : evaluation.AsLongLong.ToString(CultureInfo.InvariantCulture) };
                case CXEvalResultKind.CXEval_Float: return new { kind = "float", text = evaluation.AsDouble.ToString("R", CultureInfo.InvariantCulture) };
                case CXEvalResultKind.CXEval_StrLiteral:
                    return null;
            }
        }
        if (cursor.kind == CXCursor_VarDecl) return Value(cursor.InitExpr, depth + 1);
        if (cursor.kind == CXCursor_InitListExpr)
        {
            var values = Children(cursor).Where(c => clang.isExpression(c.kind) != 0).Select(c => Value(c, depth + 1)).ToArray();
            return values.Any(v => v == null) ? null : new { kind = "aggregate", items = values };
        }
        return null;
    }

    string[] Tokens(CXCursor cursor)
    {
        var tokens = unit.Tokenize(cursor.Extent);
        try { return tokens.ToArray().Select(token => Program.Text(token.GetSpelling(unit))).ToArray(); }
        finally { unit.DisposeTokens(tokens); }
    }

    string? Raw(CXCursor cursor)
    {
        cursor.Extent.Start.GetSpellingLocation(out var file, out _, out _, out var start);
        cursor.Extent.End.GetSpellingLocation(out var endFile, out _, out _, out var end);
        if (file != endFile || end < start || end - start > 65536) return null;
        if (!sourceFiles.TryGetValue(file, out var bytes))
        {
            var path = Program.Text(file.Name);
            if (!File.Exists(path)) return null;
            sourceFiles[file] = bytes = File.ReadAllBytes(path);
        }
        if (end > bytes.Length) return null;
        return System.Text.Encoding.UTF8.GetString(bytes, (int)start, (int)(end - start));
    }

    public void Walk(CXCursor cursor)
    {
        cursor.VisitChildren((child, parent, data) =>
        {
            if (failure != null) return CXChildVisit_Break;
            try { return Visit(child); }
            catch (Exception exception) { failure = exception; return CXChildVisit_Break; }
        }, default);
        if (failure != null) throw failure;
    }

    CXChildVisitResult Visit(CXCursor cursor)
    {
        var kind = cursor.kind;
        if (kind == CXCursor_Namespace || kind == CXCursor_LinkageSpec || kind == CXCursor_UnexposedDecl) return CXChildVisit_Recurse;
        var site = Location(cursor.Location);
        if (!Included(site.File)) return CXChildVisit_Continue;
        var name = Program.Text(cursor.Spelling);
        if (job.Mode == "values" && (kind != CXCursor_VarDecl || !macroNames.Contains(name))) return CXChildVisit_Continue;
        if (job.Mode == "templates")
        {
            var template = kind is CXCursor_ClassTemplate or CXCursor_ClassTemplatePartialSpecialization or CXCursor_FunctionTemplate or CXCursor_TypeAliasTemplateDecl;
            var templateParent = cursor.SemanticParent;
            bool dependent = template;
            for (var ancestor = templateParent; !ancestor.IsNull && ancestor.kind != CXCursor_TranslationUnit; ancestor = ancestor.SemanticParent)
                if (ancestor.kind is CXCursor_ClassTemplate or CXCursor_ClassTemplatePartialSpecialization or CXCursor_FunctionTemplate or CXCursor_TypeAliasTemplateDecl) { dependent = true; break; }
            if (dependent && clang.isDeclaration(kind) != 0 && kind != CXCursor_CXXAccessSpecifier)
                snapshot.Symbols.Add(new() { Kind = "template_" + kind.ToString().Replace("CXCursor_", ""), Name = name, Qualified = Name(cursor), Usr = Program.Text(cursor.Usr), Owner = Name(templateParent), OwnerUsr = templateParent.IsNull ? "" : Program.Text(templateParent.Usr), Site = site, Raw = Raw(cursor) });
            return kind is CXCursor_StructDecl or CXCursor_ClassDecl or CXCursor_UnionDecl or CXCursor_ClassTemplate or CXCursor_ClassTemplatePartialSpecialization or CXCursor_FunctionTemplate or CXCursor_TypeAliasTemplateDecl ? CXChildVisit_Recurse : CXChildVisit_Continue;
        }
        if (job.Mode == "probe" && (kind != CXCursor_VarDecl || !name.StartsWith("__sdk_audit_", StringComparison.Ordinal))) return CXChildVisit_Continue;
        if (kind == CXCursor_MacroDefinition)
        {
            var raw = Raw(cursor);
            var alias = raw != null && Regex.IsMatch(raw, @"^[A-Za-z_]\w*\s+[A-Za-z_]\w*\s*$");
            if (macroNames.Count != 0 && !macroNames.Contains(name) && !alias) return CXChildVisit_Continue;
            snapshot.Symbols.Add(new() { Kind = "macro", Name = name, Qualified = name, Site = site, FunctionLike = cursor.IsMacroFunctionLike, Tokens = Tokens(cursor) });
            return CXChildVisit_Continue;
        }
        var category = kind switch
        {
            CXCursor_StructDecl or CXCursor_ClassDecl or CXCursor_UnionDecl => "record",
            CXCursor_EnumDecl => "enum",
            CXCursor_TypedefDecl or CXCursor_TypeAliasDecl => "typedef",
            CXCursor_FunctionDecl => "function",
            CXCursor_CXXMethod or CXCursor_Constructor or CXCursor_Destructor or CXCursor_ConversionFunction => "method",
            CXCursor_FieldDecl => "field",
            CXCursor_EnumConstantDecl => "enumerator",
            CXCursor_VarDecl => "variable",
            _ => ""
        };
        if (category.Length == 0) return CXChildVisit_Continue;
        var usr = Program.Text(cursor.Usr);
        var identity = category + ":" + usr + ":" + site + ":" + cursor.IsDefinition;
        if (!visited.Add(identity)) return CXChildVisit_Continue;
        var parent = cursor.SemanticParent;
        var symbol = new Symbol { Kind = category, Name = name, Qualified = Name(cursor), Usr = usr, Owner = Name(parent), OwnerUsr = parent.IsNull ? "" : Program.Text(parent.Usr), Site = site,
            Type = Type(category == "typedef" ? cursor.TypedefDeclUnderlyingType : cursor.Type), Definition = cursor.IsDefinition, Anonymous = cursor.IsAnonymous };
        snapshot.Symbols.Add(symbol);
        if (category is not ("record" or "enum")) symbol.Raw = Raw(cursor);
        if (category is "record" or "enum")
        {
            if (category == "record")
            {
                for (uint i = 0; i < cursor.NumAttrs; i++)
                {
                    var attribute = cursor.GetAttr(i);
                    if (attribute.AttrKindSpelling != "Uuid") continue;
                    var text = Program.Text(attribute.PrettyPrintAttribute());
                    symbol.Uuid = Regex.Match(text, @"[0-9A-Fa-f]{8}-(?:[0-9A-Fa-f]{4}-){3}[0-9A-Fa-f]{12}").Value.ToUpperInvariant();
                }
                foreach (var child in Children(cursor).Where(c => c.kind == CXCursor_CXXBaseSpecifier)) symbol.Bases.Add(Type(child.Type));
            }
            return CXChildVisit_Recurse;
        }
        if (category is "function" or "method")
        {
            symbol.Result = Type(cursor.ResultType);
            var count = clang.Cursor_getNumArguments(cursor);
            for (uint i = 0; i < count; i++)
            {
                var parameter = clang.Cursor_getArgument(cursor, i);
                symbol.Parameters.Add(new(Program.Text(parameter.Spelling), Type(parameter.Type), Location(parameter.Location), Raw(parameter)));
            }
            if (category == "method") { symbol.Virtual = cursor.CXXMethod_IsVirtual; symbol.Pure = cursor.CXXMethod_IsPureVirtual; }
            var key = symbol.OwnerUsr + ":" + category;
            symbol.Order = orders.GetValueOrDefault(key);
            orders[key] = symbol.Order + 1;
        }
        else if (category == "field") { symbol.OffsetBits = cursor.OffsetOfField; symbol.BitWidth = cursor.IsBitField ? cursor.FieldDeclBitWidth : -1; }
        else if (category == "variable") { symbol.Const = cursor.Type.IsConstQualified; symbol.Value = Value(cursor); }
        else if (category == "enumerator") symbol.Value = new { kind = "integer", text = cursor.EnumConstantDeclValue.ToString(CultureInfo.InvariantCulture) };
        return CXChildVisit_Continue;
    }
}
